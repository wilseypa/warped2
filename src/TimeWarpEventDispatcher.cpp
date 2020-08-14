#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <limits>       // for std::numeric_limits<>::max();
#include <algorithm>    // for std::min
#include <chrono>       // for std::chrono::steady_clock
#include <iostream>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "LogicalProcess.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::EventMessage)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::Event)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::NegativeEvent)

namespace warped {

THREAD_LOCAL_SPECIFIER unsigned int TimeWarpEventDispatcher::thread_id;

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
    unsigned int num_worker_threads,
    bool is_lp_migration_on,
    unsigned int num_refresh_per_gvt,
    unsigned int num_events_per_refresh,
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
    std::unique_ptr<TimeWarpEventSet> event_set,
    std::unique_ptr<TimeWarpGVTManager> gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager,
    std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
    std::unique_ptr<TimeWarpTerminationManager> termination_manager,
    std::unique_ptr<TimeWarpStatistics> tw_stats) :
        EventDispatcher(max_sim_time), num_worker_threads_(num_worker_threads),
        is_lp_migration_on_(is_lp_migration_on),
        num_refresh_per_gvt_(num_refresh_per_gvt), num_events_per_refresh_(num_events_per_refresh), 
        comm_manager_(comm_manager), event_set_(std::move(event_set)), 
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
        termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<LogicalProcess*>>&
                                              lps) {
    initialize(lps);

    worker_threads_done_ = false;

    // Create worker threads
    std::vector<std::thread> threads;
    worker_thread_empty_schedule_queue_count_ = num_worker_threads_;
    pthread_barrier_init(&worker_thread_barrier_sync, NULL, num_worker_threads_);
    for (unsigned int i = 0; i < num_worker_threads_; ++i) {

#ifdef TIMEWARP_EVENT_LOG
        // Create the event log per thread
        event_log_.push_back(make_unique<CircularList<std::string>>());

        // Create the log files
        std::ofstream logfile;
        logfile.open (eventLogFileName(i).c_str());
        logfile << "StartTime,"
                << "Sender,"
                << "Receiver,"
                << "Timestamp,"
                << "Rollback(1=yes;0=no),"
                << "Cancelled(0=no|+ve;1=no|-ve;2=yes),"
                << "EndTime,"
                << "\n";
        logfile.close();
#endif
        worker_thread_empty_schedule_.push_back(false);
        auto thread(std::thread {&TimeWarpEventDispatcher::processEvents, this, i});
        threads.push_back(std::move(thread));
    }

    pthread_barrier_init(&termination_barrier_sync_1, NULL, num_worker_threads_+1); // This will sync all worker threads and the termination manager
    pthread_barrier_init(&termination_barrier_sync_2, NULL, 2);

    // Create manager threads
    // GVT manager
    auto sim_start = std::chrono::steady_clock::now();
    //auto gvt_thread(std::thread {&TimeWarpEventDispatcher::GVTManagerThread, this});
    //threads.push_back(std::move(gvt_thread));

    // Communication manager
    if (comm_manager_->getNumProcesses() > 1){
        // Distributed CommMgr - multiple nodes
        auto dist_comm_thread(std::thread {&TimeWarpEventDispatcher::CommunicationManagerThreadDist, this});
        threads.push_back(std::move(dist_comm_thread));

        if (comm_manager_->getID() == 0) {
            auto gvt_timer_thread(std::thread {&TimeWarpEventDispatcher::gvtTimer, this});
            threads.push_back(std::move(gvt_timer_thread));
        }
    }
    else {
        // Parallel CommMgr - only 1 node
        auto par_comm_thread(std::thread {&TimeWarpEventDispatcher::CommunicationManagerThreadPar, this});
        threads.push_back(std::move(par_comm_thread));
    }

    
    //auto comm_thread(std::thread {&TimeWarpEventDispatcher::CommunicationManagerThread, this});
    //threads.push_back(std::move(comm_thread));

    bool termination = false;

    // Termination Manager    
    while (!termination_manager_->terminationStatus()) {
        termination = true;

        //pthread_barrier_wait(&termination_barrier_sync_1); // Have all worker threads sync up, so that we can test for termination
        //comm_manager_->barrierPause();
        //pthread_barrier_wait(&termination_barrier_sync_2); // Suspend Comm Manager, so that no messages can be passed
        //comm_manager_->barrierResume();
        //pthread_barrier_wait(&termination_barrier_sync_1); // Need to wait for the comm manager to suspend before the worker threads can check their schedule queues
        // Worker threads are checking their schedule queues between these two syncs (the line above ^^^^ and below VVVVV)
        //pthread_barrier_wait(&termination_barrier_sync_1); // Wait for all worker threads to check their schedule queues before making a desiscion on termination
        //pthread_barrier_wait(&termination_barrier_sync_2); // Resume Comm Manager
       
        // If this node is passive, then we set this node to terminate and send a termination token to the next node
        if (termination_manager_->nodePassive()){
            termination = true;
            termination_manager_->setTerminate(termination);         
            termination_manager_->sendTerminationToken(State::PASSIVE, comm_manager_->getID(), 0);       
        } else {
            termination = false;
            termination_manager_->setTerminate(termination);
        }
        //pthread_barrier_wait(&termination_barrier_sync_1); // Need to allow the termination manager to set termination status, before worker threads test termination status
    }  

    comm_manager_->waitForAllProcesses();
    auto sim_stop = std::chrono::steady_clock::now();

    double num_seconds = double((sim_stop - sim_start).count()) *
                std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;


    if (comm_manager_->getID() == 0) {
        std::cout << "\nSimulation completed in " << num_seconds << " second(s)" << "\n\n";
    }

    for (auto& t: threads) {
        t.join();
    }

    unsigned int gvt = (unsigned int)-1;
    for (unsigned int current_lp_id = 0; current_lp_id < num_local_lps_; current_lp_id++) {
        unsigned int num_committed = event_set_->fossilCollect(gvt, current_lp_id);
        tw_stats_->upCount(EVENTS_COMMITTED, thread_id, num_committed);
    }

    tw_stats_->calculateStats();

    if (comm_manager_->getID() == 0) {
        tw_stats_->writeToFile(num_seconds);
        tw_stats_->printStats();
    }
}

void TimeWarpEventDispatcher::GVTManagerThread(){
    unsigned int gvt = 0;

    // Only 1 node
    if (comm_manager_->getNumProcesses() == 1) {
        while(1){
            std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
            
            gvt_manager_->getReportGVTFlagLock();
            gvt_manager_->setReportGVT(true);
            gvt_manager_->getReportGVTFlagUnlock();

            gvt_manager_->workerThreadGVTBarrierSync();
            gvt_manager_->workerThreadGVTBarrierSync();

            //gvt_manager_->getReportGVTFlagLock();
            gvt_manager_->setReportGVT(false);
            //gvt_manager_->getReportGVTFlagUnlock();

            //gvt_manager_->progressGVT(); // Calculate min gvt

            if (gvt == std::numeric_limits<unsigned int>::max()) break;
            if (gvt_manager_->gvtUpdated() && !termination_manager_->terminationStatus()) {
                //gvt_manager_->accessGVTLockShared();
                gvt = gvt_manager_->getGVT();
                //gvt_manager_->accessGVTUnlockShared();
                onGVT(gvt);
            }

            
        }
    } 
    // Distributed proccess, main node
    /*else if (comm_manager_->getID() == 0){
        while(1){


        }
    } */
    // Distributed proccess, not the main node
    /*else if (comm_manager_->getID() != 0){
        while(1){


        }
    }*/
}

void TimeWarpEventDispatcher::onGVT(unsigned int gvt) {
    auto malloc_info = mallinfo();
    int m = malloc_info.uordblks;

    uint64_t mem = m;
    if (m < 0) {
        mem = (uint64_t)2*1024*1024*1024 + (uint64_t)m;
    }

    if (comm_manager_->getID() == 0) {
        std::cout << "GVT: " << gvt << std::endl;
    }

    uint64_t c = tw_stats_->upCount(GVT_CYCLES, num_worker_threads_);
    tw_stats_->updateAverage(AVERAGE_MAX_MEMORY, mem, c);
}



void TimeWarpEventDispatcher::CommunicationManagerThreadPar(){
std::cout << "1 Node Comm Mgr" << std::endl;
    unsigned int gvt = 0;
    unsigned int temp_local_min;
    unsigned int next_gvt;
    while(1){
        //std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        if (gvt_manager_->readyToStart()){
            gvt_manager_->getReportGVTFlagLock();
            gvt_manager_->setReportGVT(true);
            gvt_manager_->getReportGVTFlagUnlock();
            gvt_manager_->workerThreadGVTBarrierSync();

            gvt_manager_->progressGVT(temp_local_min);
            next_gvt = gvt_manager_->getNextGVT();
            comm_manager_->minAllReduceUint(&temp_local_min, &next_gvt);
            gvt_manager_->setNextGVT(next_gvt);

            if (gvt == std::numeric_limits<unsigned int>::max()) break;
            if (gvt_manager_->gvtUpdated()) {
                gvt = gvt_manager_->getGVT();
                onGVT(gvt);
            }
        }
    }
}

void TimeWarpEventDispatcher::gvtTimer(){
std::cout << "GVT TIMER NODE = " << comm_manager_->getID() << std::endl;
bool test = false;
   
    bool temp_lock = false;
    int skip_gvt_cycle = 5;

    while(1){
	    if (!test) {
	    //	std::cout << "GVT TIMER WHILE LOOP" << std::endl;
		    test = true;
	    }
        for (int loop = 0; loop < skip_gvt_cycle; loop++){
                std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        if (gvt_manager_->readyToStart() && temp_lock){
	    if (test) {
	//	    std::cout << "GVT TIMER READY TO START" << std::endl;
		    test = false;
	    }
            gvt_manager_->getReportGVTFlagLock();
            gvt_manager_->setReportGVT(true);
            gvt_manager_->getReportGVTFlagUnlock();
            
	    gvt_manager_->triggerSynchGVTCalculation();
        }
//	while(1){
  //          std::this_thread::sleep_for(std::chrono::seconds(1));
	    
	    host_node_done_lock_.lock();
	    temp_lock = host_node_done_;
	    host_node_done_lock_.unlock();
  //  	    if (temp_lock) break;
//	}
    }
}

void TimeWarpEventDispatcher::CommunicationManagerThreadDist(){
std::cout << "Many Node Comm Mgr NODE = " << comm_manager_->getID() << std::endl;
    unsigned int gvt = 0;
    unsigned int temp_local_min;
    unsigned int next_gvt;
bool test = false;
    // Will run until this thread is destroyed
    while(!termination_manager_->terminationStatus()){
std::cout << "Comm Top Test = " << test << " Node = " << comm_manager_->getID() << std::endl;	 
        while(1){
            if (!test) {
                test = true;
                std::cout << "WHILE LOOP NODE = " << comm_manager_->getID() << std::endl;            
            }
            
            comm_manager_->handleMessages();
    	    
	    // Report GVT Flag is updated whenever a message is recieved. Look at receiveGVTSynchTrigger() in TWSynchronousGVTManager
            
            if (gvt_manager_->getGVTFlag()){
                while (gvt_manager_->getTokenSendConfirmation()){
                    comm_manager_->handleMessages();
                }
                gvt_manager_->setTokenSendConfirmation(false);
	            break;
	        }
        }

		std::cout << "PROGRESSGVT 0 NODE = " << comm_manager_->getID() << std::endl;
        host_node_done_lock_.lock();
	    host_node_done_ = false;
	    host_node_done_lock_.unlock();

		std::cout << "PROGRESSGVT 1 NODE = " << comm_manager_->getID() << std::endl;
        gvt_manager_->progressGVT(temp_local_min);
		std::cout << "PROGRESSGVT 2 NODE = " << comm_manager_->getID() << std::endl;
        MPI_Barrier(MPI_COMM_WORLD);
		std::cout << "PROGRESSGVT 3 NODE = " << comm_manager_->getID() << std::endl;
        next_gvt = gvt_manager_->getNextGVT();
        comm_manager_->minAllReduceUint(&temp_local_min, &next_gvt);
        gvt_manager_->setNextGVT(next_gvt);
		std::cout << "PROGRESSGVT 4 NODE = " << comm_manager_->getID() << std::endl;
        if (gvt == std::numeric_limits<unsigned int>::max()) break;
        if (gvt_manager_->gvtUpdated()) {
            gvt = gvt_manager_->getGVT();
            onGVT(gvt);
        }
		test = false;
		std::cout << "PROGRESSGVT 5 NODE = " << comm_manager_->getID() << std::endl;
        host_node_done_lock_.lock();
	    host_node_done_ = true;
	    host_node_done_lock_.unlock();
    }	   

    std::cout << "COMM TERMINATED NODE = " << comm_manager_->getID() << std::endl;
}


void TimeWarpEventDispatcher::processEvents(unsigned int id) {
    thread_id = id;
    unsigned int min_timestamp = std::numeric_limits<unsigned int>::max();
    unsigned int gvt = 0;
    bool report_gvt = false;
    bool test = false;

    //auto gvt_start = std::chrono::steady_clock::now();
    //auto gvt_stop = std::chrono::steady_clock::now();
    //double gvt_wait = 0;

    //auto gvt_barr_start = std::chrono::steady_clock::now();
    //auto gvt_barr_stop = std::chrono::steady_clock::now();
    //double gvt_barr_wait = 0;


#ifdef TIMEWARP_EVENT_LOG
    auto epoch = std::chrono::steady_clock::now();
#endif

    std::shared_ptr<Event> event_input_queue_next;
    std::shared_ptr<Event> event_input_queue_head;
    //std::shared_ptr<Event> event_next;
    event_set_->refreshScheduleQueue(thread_id, with_read_lock);
    std::shared_ptr<Event> event;// = event_set_->getEvent(thread_id, with_input_queue_check);

    while(1){      
        report_gvt = gvt_manager_->getGVTFlag();
        // Don't need to grab an event twice if we are reporting gvt
        if (!report_gvt) event = event_set_->getEvent(thread_id, with_input_queue_check);  
        
        if (event == nullptr || report_gvt){
            //gvt_start = std::chrono::steady_clock::now();
            gvt_manager_->workerThreadGVTBarrierSync();
            //gvt_stop = std::chrono::steady_clock::now();
            //gvt_wait = double((gvt_stop - gvt_start).count()) + gvt_wait;

            // fossil collection
            unsigned int fossil_current_lp_id;
            LogicalProcess* fossil_current_lp;

            gvt = gvt_manager_->getGVT();

            for (unsigned int t = 0; t < worker_thread_input_queue_map_[thread_id].size(); t++){
                fossil_current_lp_id = worker_thread_input_queue_map_[thread_id][t];
                fossil_current_lp = lps_by_name_[local_lp_name_by_id_[fossil_current_lp_id]];
                if (gvt > fossil_current_lp->last_fossil_collect_gvt_) {
                    fossil_current_lp->last_fossil_collect_gvt_ = gvt;

                    // Fossil collect all queues for this lp
                    twfs_manager_->fossilCollect(gvt, fossil_current_lp_id);
                    output_manager_->fossilCollect(gvt, fossil_current_lp_id);

                    unsigned int event_fossil_collect_time =
                        state_manager_->fossilCollect(gvt, fossil_current_lp_id);

                    unsigned int num_committed =
                        event_set_->fossilCollect(event_fossil_collect_time, fossil_current_lp_id);

                    tw_stats_->upCount(EVENTS_COMMITTED, thread_id, num_committed);
                }
            }

            event_set_->refreshScheduleQueue(thread_id, without_read_lock);
            event = event_set_->getEvent(thread_id, without_input_queue_check);            
            if (event != nullptr) {
                min_timestamp = event->timestamp();
            }
            else {
                min_timestamp = std::numeric_limits<unsigned int>::max();
            }
	    std::cout << " MIN TIME STAMP = " << min_timestamp << " NODE = " << comm_manager_->getID() << " Thread = " << thread_id << std::endl;
            gvt_manager_->reportThreadMin(min_timestamp, thread_id);

            gvt_manager_->workerThreadGVTBarrierSync();
            //gvt_barr_stop = std::chrono::steady_clock::now();
            //gvt_barr_wait = double((gvt_barr_stop - gvt_start).count()) + gvt_barr_wait;

            if (gvt == std::numeric_limits<unsigned int>::max()){
                if (!termination_manager_->threadPassive(thread_id)) {
                    termination_manager_->setThreadPassive(thread_id);
                }
//double num_seconds = gvt_wait *
//                std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
//double num_barrier_seconds = gvt_barr_wait *
//                std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
//std::cout << "Time Wasted on Barrier = " << num_seconds << std::endl;
//std::cout << "Time Wasted on GVT = " << num_barrier_seconds << std::endl;

                // One Final Check for events
                event_set_->refreshScheduleQueue(thread_id, without_read_lock);
                event = event_set_->getEvent(thread_id, without_input_queue_check); 
//if (event != nullptr)  std::cout << event->timestamp() << " 2 " << event->receiverName() << std::endl;     
                if (event != nullptr) {
                    std::cout << "UNPROCESSED EVENT" << std::endl;
                }
                break;
            }
        }

        if (event != nullptr){
        for (unsigned int i = 0; i < num_refresh_per_gvt_; i++){
            for (unsigned int j = 0; j < num_events_per_refresh_; j++){
                test = false;
#ifdef TIMEWARP_EVENT_LOG
            // Event stat - start processing time, sender name, receiver name, timestamp
            auto event_stats = std::to_string((std::chrono::steady_clock::now() - epoch).count());
            event_stats     += "," + event->sender_name_;
            event_stats     += "," + event->receiverName();
            event_stats     += "," + std::to_string(event->timestamp());
#endif
                assert(comm_manager_->getNodeID(event->receiverName()) == comm_manager_->getID());
                unsigned int current_lp_id = local_lp_id_by_name_[event->receiverName()];
                LogicalProcess* current_lp = lps_by_name_[event->receiverName()];

                // Get the last processed event so we can check for a rollback
                auto last_processed_event = event_set_->lastProcessedEvent(current_lp_id);

                event_set_->getInputQueueHead(current_lp_id, event_input_queue_head, event_input_queue_next); // Input Queue Read Lock

                if (event_input_queue_head != nullptr){
                    if (event->timestamp() > event_input_queue_head->timestamp()){
                        rollback(event_input_queue_head);
                        event_set_->changedScheduledEventPtr (current_lp_id, event_input_queue_head);
                        test = true;
                    }
                    else {
                        event_input_queue_head = event;
                    }
                }
                else {
                    event_input_queue_head = event;
                }

                // The rules with event processing
                //      1. Negative events are given priority over positive events if they both exist
                //          in the lps input queue
                //      2. We assume that if we have a negative message, then we also have the positive
                //          message either in the input queue or in the processed queue. If the positive
                //          event is in the processed queue, then a rollback will occur and both events
                //          will end up in the input queue.
                //      3. When a negative event is taken from the schedule queue, it will be cancelled
                //          with it's corresponding negative message in the input queue. A rollback
                //          may occur first.
                //      4. When a positive event is taken from the schedule queue, it will always be
                //          processed. A rollback may occur first if it is a straggler.

                // A rollback can occur in two situations:
                //      1. We get an event that is strictly less than the last processed event.
                //      2. We get an event that is equal to the last processed event and is negative.
                if (!test) {
                if (last_processed_event &&
                        ((*event_input_queue_head < *last_processed_event) ||
                            ((*event_input_queue_head == *last_processed_event) &&
                            (event_input_queue_head->event_type_ == EventType::NEGATIVE)))) {
                    rollback(event_input_queue_head);
#ifdef TIMEWARP_EVENT_LOG
                event_stats += ",1"; // Event stats - rollback

            } else {
                event_stats += ",0"; // Event stats - no rollback
#endif
                }
                }

                // Check to see if event is NEGATIVE and cancel
                if (event_input_queue_head->event_type_ == EventType::NEGATIVE) {
                    event_set_->acquireInputQueueLock(current_lp_id);
                    bool found = event_set_->cancelEvent(current_lp_id, event_input_queue_head);
                    event_set_->startScheduling(current_lp_id);
                    event_set_->releaseInputQueueLock(current_lp_id);


                    if (found) {
                        tw_stats_->upCount(CANCELLED_EVENTS, thread_id);
#ifdef TIMEWARP_EVENT_LOG
                        event_stats += ",2"; // Event stats - negative event, cancelled
                    } else {
                        event_stats += ",1"; // Event stats - negative event, not cancelled
#endif
                    }

#ifdef TIMEWARP_EVENT_LOG
                // Event stats - event processing time
                auto end_event = std::chrono::steady_clock::now();
                event_stats +=  "," + std::to_string((end_event-epoch).count());

                // Event stats - store it
                event_stats += "\n";
                event_log_[thread_id]->insert(event_stats);
#endif       
       
                    // Grab the next event, make sure to not grab another event from the scedule queue if this is the last iteration of num_events_per_refresh
                    if (j < num_events_per_refresh_-1){
                        event = event_set_->getEvent(thread_id, without_input_queue_check);

                        if (event == nullptr) {
                            break;
                        }
                    }

                    continue;

#ifdef TIMEWARP_EVENT_LOG
                } else {
                    event_stats += ",0"; // Event stats - positive event
#endif
                }

                // process event and get new events
                auto new_events = current_lp->receiveEvent(*event_input_queue_head);
                tw_stats_->upCount(EVENTS_PROCESSED, thread_id);

                // Save state
                state_manager_->saveState(event_input_queue_head, current_lp_id, current_lp);
                // Send new events
                sendEvents(event_input_queue_head, new_events, current_lp_id, current_lp);
#ifdef TIMEWARP_EVENT_LOG
                // Event stats - event processing time
                auto end_event = std::chrono::steady_clock::now();
                event_stats +=  "," + std::to_string((end_event-epoch).count());

                // Event stats - store it
                event_stats += "\n";
                event_log_[thread_id]->insert(event_stats);
#endif

                // Move the next event from lp into the schedule queue
                // Also transfer old event to processed queue
//std::cout << "Main 3 " << event_input_queue_next->timestamp() << " " << event_input_queue_next->receiverName() << std::endl;

                event_set_->acquireInputQueueLock(current_lp_id);
                event_set_->replenishScheduler(current_lp_id, event_input_queue_next);
                event_set_->releaseInputQueueLock(current_lp_id);

#ifdef TIMEWARP_EVENT_LOG
                // Write event statistics to the log file
                std::ofstream logfile;
                logfile.open( eventLogFileName(thread_id).c_str(),
                                        std::ofstream::out | std::ofstream::app );
                while (event_log_[thread_id]->size()) {
                    logfile << event_log_[thread_id]->pop_front();
                }
                logfile.close();
#endif
                // Grab the next event, make sure to not grab another event from the scedule queue if this is the last iteration of num_events_per_refresh
                if (j < num_events_per_refresh_-1){
                    event = event_set_->getEvent(thread_id, without_input_queue_check);
                    if (event == nullptr) {
                        break;
                    }
                }
            } 
            
            // Grab the next event, make sure to not grab another event from the scedule queue if this is the last iteration of num_refresh_per_gvt
            if (i < num_refresh_per_gvt_-1){
                event_set_->refreshScheduleQueue(thread_id, with_read_lock);
                event = event_set_->getEvent(thread_id, without_input_queue_check);
                if (event == nullptr) {
                    break;
                }
            }

        } 
       
    }
    }
std::cout << "Work - End" << std::endl;
}

void TimeWarpEventDispatcher::receiveEventMessage(std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
    assert(msg->event != nullptr);

    tw_stats_->upCount(TOTAL_EVENTS_RECEIVED, thread_id);

    termination_manager_->updateMsgCount(-1);
    gvt_manager_->receiveEventUpdate(msg->event, msg->color_);

    sendLocalEvent(msg->event);
}

void TimeWarpEventDispatcher::sendEvents(std::shared_ptr<Event> source_event,
    std::vector<std::shared_ptr<Event>> new_events, unsigned int sender_lp_id,
    LogicalProcess *sender_lp) {

    for (auto& e: new_events) {

        // Make sure not to send any events past max time so we can terminate simulation
        if (e->timestamp() <= max_sim_time_) {
            e->sender_name_ = sender_lp->name_;
            e->send_time_ = source_event->timestamp();
            e->generation_ = sender_lp->generation_++;

            // Save sent events so that they can be sent as anti-messages in the case of a rollback
            output_manager_->insertEvent(source_event, e, sender_lp_id);

            unsigned int node_id = comm_manager_->getNodeID(e->receiverName());
            if (node_id == comm_manager_->getID()) {
                // Local event
                sendLocalEvent(e);
                tw_stats_->upCount(LOCAL_POSITIVE_EVENTS_SENT, thread_id);
            } else {
                // Remote event
                enqueueRemoteEvent(e, node_id);
                tw_stats_->upCount(REMOTE_POSITIVE_EVENTS_SENT, thread_id);
            }
        }
    }
}

void TimeWarpEventDispatcher::sendLocalEvent(std::shared_ptr<Event> event) {
    unsigned int receiver_lp_id = local_lp_id_by_name_[event->receiverName()];

    // NOTE: Event is assumed to be less than the maximum simulation time.
    event_set_->acquireInputQueueLock(receiver_lp_id);
    auto status = event_set_->insertEvent(receiver_lp_id, event);
    event_set_->releaseInputQueueLock(receiver_lp_id);

    if (status == InsertStatus::StarvedObject) {
        tw_stats_->upCount(EVENTS_FOR_STARVED_OBJECTS, thread_id);
    } else if (status == InsertStatus::SchedEventSwapSuccess) {
        tw_stats_->upCount(SCHEDULED_EVENT_SWAPS_SUCCESS, thread_id);
    } else if (status == InsertStatus::SchedEventSwapFailure) {
        tw_stats_->upCount(SCHEDULED_EVENT_SWAPS_FAILURE, thread_id);
    }
}

void TimeWarpEventDispatcher::cancelEvents(
            std::unique_ptr<std::vector<std::shared_ptr<Event>>> events_to_cancel) {

    if (events_to_cancel->empty()) return;

    // NOTE: events to cancel are in order from LARGEST to SMALLEST so we send from the back
    do {
        auto event = events_to_cancel->back();

        // NOTE: this is a copy the positive event
        auto neg_event = std::make_shared<NegativeEvent>(event);
        events_to_cancel->pop_back();

        // Make sure not to send any events past max time so that events can be exhausted and we
        // can terminate the simulation.
        if (event->timestamp() <= max_sim_time_) {
            unsigned int receiver_node_id = comm_manager_->getNodeID(event->receiverName());
            if (receiver_node_id == comm_manager_->getID()) {
                sendLocalEvent(neg_event);
                tw_stats_->upCount(LOCAL_NEGATIVE_EVENTS_SENT, thread_id);
            } else {
                enqueueRemoteEvent(neg_event, receiver_node_id);
                tw_stats_->upCount(REMOTE_NEGATIVE_EVENTS_SENT, thread_id);
            }
        }

    } while (!events_to_cancel->empty());
}

void TimeWarpEventDispatcher::rollback(std::shared_ptr<Event> straggler_event) {

    unsigned int local_lp_id = local_lp_id_by_name_[straggler_event->receiverName()];
    LogicalProcess* current_lp = lps_by_name_[straggler_event->receiverName()];

    // Statistics count
    if (straggler_event->event_type_ == EventType::POSITIVE) {
        tw_stats_->upCount(PRIMARY_ROLLBACKS, thread_id);
    } else {
        tw_stats_->upCount(SECONDARY_ROLLBACKS, thread_id);
    }

    // Rollback output file stream. XXX so far this is not used by any models
    twfs_manager_->rollback(straggler_event, local_lp_id);

    // We have major problems if we are rolling back past the GVT
    //gvt_manager_->accessGVTLockShared();
    assert(straggler_event->timestamp() >= gvt_manager_->getGVT());
    //gvt_manager_->accessGVTUnlockShared();

    // Move processed events larger  than straggler back to input queue.
    event_set_->acquireInputQueueLock(local_lp_id);
    event_set_->rollback(local_lp_id, straggler_event);
    event_set_->releaseInputQueueLock(local_lp_id);

    // Restore state by getting most recent saved state before the straggler and coast forwarding.
    auto restored_state_event = state_manager_->restoreState(straggler_event, local_lp_id,
        current_lp);
    assert(restored_state_event);
    assert(*restored_state_event < *straggler_event);

    // Send anti-messages
    auto events_to_cancel = output_manager_->rollback(straggler_event, local_lp_id);
    if (events_to_cancel != nullptr) {
        cancelEvents(std::move(events_to_cancel));
    }

    coastForward(straggler_event, restored_state_event);
}

void TimeWarpEventDispatcher::coastForward(std::shared_ptr<Event> straggler_event, 
                                            std::shared_ptr<Event> restored_state_event) {

    LogicalProcess* lp = lps_by_name_[straggler_event->receiverName()];
    unsigned int current_lp_id = local_lp_id_by_name_[straggler_event->receiverName()];

    auto events = event_set_->getEventsForCoastForward(current_lp_id, straggler_event,
        restored_state_event);

    // NOTE: events are in order from LARGEST to SMALLEST, so reprocess backwards
    for (auto event_riterator = events->rbegin();
                    event_riterator != events->rend(); event_riterator++) {

        assert(**event_riterator <= *straggler_event);

        // This just updates state, ignore new events
        lp->receiveEvent(**event_riterator);

        tw_stats_->upCount(COAST_FORWARDED_EVENTS, thread_id);

        // NOTE: Do not send any new events
        // NOTE: All coast forward events are already in processed queue, they were never removed.
    }
}

void
TimeWarpEventDispatcher::initialize(const std::vector<std::vector<LogicalProcess*>>& lps) {

    thread_id = num_worker_threads_;

    num_local_lps_ = 0;
    for (auto& p: lps) {
        num_local_lps_ += p.size();
    }

    event_set_->initialize(lps, num_local_lps_, is_lp_migration_on_, num_worker_threads_);

    std::vector<unsigned int> empty_vector;
    unsigned int worker_thread_number = 0;
    unsigned int lp_id = 0;
    for (auto& partition : lps) {
        worker_thread_input_queue_map_.push_back(empty_vector);
        for (auto& lp : partition) {
            lps_by_name_[lp->name_] = lp;
            local_lp_id_by_name_[lp->name_] = lp_id;
            local_lp_name_by_id_[lp_id] = lp->name_;
            worker_thread_input_queue_map_[worker_thread_number].push_back(lp_id);
            lp_id++;
        }
        worker_thread_number++;
    }

    event_set_->setLocalLPIdByName(local_lp_id_by_name_);
    event_set_->setWorkerThreadInputQueueMap(worker_thread_input_queue_map_);

    // Creates the state queues, output queues, and filestream queues for each local lp
    state_manager_->initialize(num_local_lps_);
    output_manager_->initialize(num_local_lps_);
    twfs_manager_->initialize(num_local_lps_);

    // Register message handlers
    gvt_manager_->initialize();
    termination_manager_->initialize(num_worker_threads_);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpEventDispatcher, receiveEventMessage, EventMessage);

    // Initialize statistics data structures
    tw_stats_->initialize(num_worker_threads_, num_local_lps_);

    comm_manager_->waitForAllProcesses();

    // Send local initial events and enqueue remote initial events
    auto initial_event = std::make_shared<InitialEvent>();
    for (auto& partition : lps) {
        for (auto& lp : partition) {
            unsigned int lp_id = local_lp_id_by_name_[lp->name_];
            auto new_events = lp->initializeLP();
            sendEvents(initial_event, new_events, lp_id, lp);
            state_manager_->saveState(initial_event, lp_id, lp);
        }
    }

    int64_t total_msg_count;
    while (true) {
        comm_manager_->flushMessages();
        comm_manager_->handleMessages();
        int64_t local_msg_count = gvt_manager_->getMessageCount();
        comm_manager_->sumAllReduceInt64(&local_msg_count, &total_msg_count);
        if(total_msg_count == 0)
            break;
    }

    comm_manager_->handleMessages();
    comm_manager_->waitForAllProcesses();
}

// XXX This is never used by any models
FileStream& TimeWarpEventDispatcher::getFileStream(LogicalProcess *lp,
    const std::string& filename, std::ios_base::openmode mode, std::shared_ptr<Event> this_event) {

    unsigned int local_lp_id = local_lp_id_by_name_[lp->name_];

    return *twfs_manager_->getFileStream(filename, mode, local_lp_id, this_event);
}

void TimeWarpEventDispatcher::enqueueRemoteEvent(std::shared_ptr<Event> event,
    unsigned int receiver_id) {

    if (event->timestamp() <= max_sim_time_) {

        Color color = gvt_manager_->sendEventUpdate(event);

        auto event_msg = make_unique<EventMessage>(comm_manager_->getID(), receiver_id, event, color);
        termination_manager_->updateMsgCount(1);
        comm_manager_->insertMessage(std::move(event_msg));
    }
}

} // namespace warped


