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
    unsigned int k0,
    unsigned int k1,
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
        k0_(k0), k1_(k1), 
        comm_manager_(comm_manager), event_set_(std::move(event_set)), 
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
        termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}

void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<LogicalProcess*>>&
                                              lps) {
    initialize(lps);

    // Create worker threads
    std::vector<std::thread> threads;
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

        auto thread(std::thread {&TimeWarpEventDispatcher::processEvents, this, i});
        threads.push_back(std::move(thread));
    }


    pthread_barrier_init(&termination_barrier_sync_1, NULL, num_worker_threads_+2);
    pthread_barrier_init(&termination_barrier_sync_2, NULL, 2);

    // Create manager threads
    // GVT manager
    auto sim_start = std::chrono::steady_clock::now();
    auto gvt_thread(std::thread {&TimeWarpEventDispatcher::GVTManagerThread, this});
    threads.push_back(std::move(gvt_thread));

    // Communication manager
    auto comm_thread(std::thread {&TimeWarpEventDispatcher::CommunicationManagerThread, this});
    threads.push_back(std::move(comm_thread));

    // Termination Manager    
    while (!termination_manager_->terminationStatus()) {

        termination_manager_->setTerminate(true);

        pthread_barrier_wait(&termination_barrier_sync_1);
        comm_manager_->barrierPause();
        pthread_barrier_wait(&termination_barrier_sync_2);
        comm_manager_->barrierResume();
        pthread_barrier_wait(&termination_barrier_sync_1);
        pthread_barrier_wait(&termination_barrier_sync_1);
        pthread_barrier_wait(&termination_barrier_sync_2);
        
        // If this node is passive, then we set this node to terminate and send a termination token to the next node
        if (termination_manager_->nodePassive()){
            termination_manager_->setTerminate(true);         
            termination_manager_->sendTerminationToken(State::PASSIVE, comm_manager_->getID(), 0);       
        } else {
            termination_manager_->setTerminate(false);
        }
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

            //std::this_thread::sleep_for(std::chrono::nanoseconds(10));

            gvt_manager_->progressGVT();

            if (gvt_manager_->gvtUpdated()) {
                gvt = gvt_manager_->getGVT();
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

void TimeWarpEventDispatcher::CommunicationManagerThread(){
    // Will run until this thread is destroyed
    while(true){
        comm_manager_->handleMessages();
        
        if (comm_manager_->barrierHoldStatus()){
            pthread_barrier_wait(&termination_barrier_sync_2);
            pthread_barrier_wait(&termination_barrier_sync_2);
        }
    }
}

void TimeWarpEventDispatcher::processEvents(unsigned int id) {

    thread_id = id;
    unsigned int local_gvt_flag;
    unsigned int gvt = 0;

#ifdef TIMEWARP_EVENT_LOG
    auto epoch = std::chrono::steady_clock::now();
#endif


    // GetEvent needs to be modyfied to do readLock
    // Had to add getGVTFlag() and workerThreadGVTBarrierSync() to the base TWGVTManager then I had to add it to the Asynchronous version. Not sure how to avoid doing that


    std::shared_ptr<Event> event = event_set_->getEvent(thread_id);
    if (event == nullptr) {
        goto refresh_schedule_queue;
    }

start_of_process_event:
    while(1){
        for (unsigned int i = 0; i < k0_; i++){
            for (unsigned int j = 0; j < k1_; j++){



            }
            event = event_set_->getEvent(thread_id);
            if (event == nullptr) {
                goto refresh_schedule_queue;
            }
        }
        if (gvt_manager_->getGVTFlag()){
            gvt_manager_->workerThreadGVTBarrierSync();
            // fossil collection
            event = event_set_->getEvent(thread_id);
            // estGvtEst() ??????
            gvt_manager_->workerThreadGVTBarrierSync();
        }

    }

refresh_schedule_queue:
    // if (refresh queue and grab event success) {
        goto start_of_process_event;
    // } else {
            pthread_barrier_wait(&termination_barrier_sync_1);


    // }








    while (!termination_manager_->terminationStatus()) {

        std::shared_ptr<Event> event = event_set_->getEvent(thread_id);
        if (event != nullptr) {

#ifdef TIMEWARP_EVENT_LOG
            // Event stat - start processing time, sender name, receiver name, timestamp
            auto event_stats = std::to_string((std::chrono::steady_clock::now() - epoch).count());
            event_stats     += "," + event->sender_name_;
            event_stats     += "," + event->receiverName();
            event_stats     += "," + std::to_string(event->timestamp());
#endif

            // If needed, report event for this thread so GVT can be calculated
            auto lowest_timestamp = event->timestamp();

#ifdef PARTIALLY_SORTED_LADDER_QUEUE
            lowest_timestamp = event_set_->lowestTimestamp(thread_id);
#endif

            gvt_manager_->reportThreadMin(lowest_timestamp, thread_id, local_gvt_flag);

            // Make sure that if this thread is currently seen as passive, we update it's state
            //  so we don't terminate early.
            if (termination_manager_->threadPassive(thread_id)) {
                termination_manager_->setThreadActive(thread_id);
            }

            assert(comm_manager_->getNodeID(event->receiverName()) == comm_manager_->getID());
            unsigned int current_lp_id = local_lp_id_by_name_[event->receiverName()];
            LogicalProcess* current_lp = lps_by_name_[event->receiverName()];

            // Get the last processed event so we can check for a rollback
            auto last_processed_event = event_set_->lastProcessedEvent(current_lp_id);

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

            if (last_processed_event &&
                    ((*event < *last_processed_event) ||
                        ((*event == *last_processed_event) &&
                         (event->event_type_ == EventType::NEGATIVE)))) {
                rollback(event);
#ifdef TIMEWARP_EVENT_LOG
                event_stats += ",1"; // Event stats - rollback

            } else {
                event_stats += ",0"; // Event stats - no rollback
#endif
            }

            // Check to see if event is NEGATIVE and cancel
            if (event->event_type_ == EventType::NEGATIVE) {
                event_set_->acquireInputQueueLock(current_lp_id);
                bool found = event_set_->cancelEvent(current_lp_id, event);
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
                continue;

#ifdef TIMEWARP_EVENT_LOG
            } else {
                event_stats += ",0"; // Event stats - positive event
#endif
            }

            // process event and get new events
            auto new_events = current_lp->receiveEvent(*event);

            tw_stats_->upCount(EVENTS_PROCESSED, thread_id);

            // Save state
            state_manager_->saveState(event, current_lp_id, current_lp);

            // Send new events
            sendEvents(event, new_events, current_lp_id, current_lp);

#ifdef TIMEWARP_EVENT_LOG
            // Event stats - event processing time
            auto end_event = std::chrono::steady_clock::now();
            event_stats +=  "," + std::to_string((end_event-epoch).count());

            // Event stats - store it
            event_stats += "\n";
            event_log_[thread_id]->insert(event_stats);
#endif

            // Check for recent gvt update
            gvt = gvt_manager_->getGVT();
            if (gvt > current_lp->last_fossil_collect_gvt_) {
                current_lp->last_fossil_collect_gvt_ = gvt;

                // Fossil collect all queues for this lp
                twfs_manager_->fossilCollect(gvt, current_lp_id);
                output_manager_->fossilCollect(gvt, current_lp_id);

                unsigned int event_fossil_collect_time =
                    state_manager_->fossilCollect(gvt, current_lp_id);

                unsigned int num_committed =
                    event_set_->fossilCollect(event_fossil_collect_time, current_lp_id);

                tw_stats_->upCount(EVENTS_COMMITTED, thread_id, num_committed);

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
            }

            // Move the next event from lp into the schedule queue
            // Also transfer old event to processed queue
            event_set_->acquireInputQueueLock(current_lp_id);
            event_set_->replenishScheduler(current_lp_id);
            event_set_->releaseInputQueueLock(current_lp_id);

        } else {
            // This thread no longer has anything to do because it's schedule queue is empty.
            if (!termination_manager_->threadPassive(thread_id)) {
                termination_manager_->setThreadPassive(thread_id);
            }

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

            // We must have this so that the GVT calculations can continue with passive threads.
            // Just report infinite for a time.
            gvt_manager_->reportThreadMin((unsigned int)-1, thread_id, local_gvt_flag);
        }
    }
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

    // Make sure to track sends if we are in the middle of a GVT calculation.
    gvt_manager_->reportThreadSendMin(event->timestamp(), thread_id);

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
    assert(straggler_event->timestamp() >= gvt_manager_->getGVT());

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

    unsigned int lp_id = 0;
    for (auto& partition : lps) {
        for (auto& lp : partition) {
            lps_by_name_[lp->name_] = lp;
            local_lp_id_by_name_[lp->name_] = lp_id;
            lp_id++;
        }
    }

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

        gvt_manager_->reportThreadSendMin(event->timestamp(), thread_id);
    }
}

} // namespace warped


