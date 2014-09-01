#include "TimeWarpEventSet.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

void TimeWarpEventSet::initialize( unsigned int num_of_objects, 
                                   unsigned int num_of_schedulers,
                                   unsigned int num_of_worker_threads ) {

    num_of_objects_ = num_of_objects;
    num_of_schedulers_ = num_of_schedulers;

    /* Create the unprocessed and processed queues and their locks.
       Also create the uprocessed queue to schedule queue map. */
    for (unsigned int obj_id = 0; obj_id < num_of_objects; obj_id++) {
        unprocessed_queue_.push_back(
            new std::multiset<const std::shared_ptr<Event>, compareEvents>);
        processed_queue_.push_back(
            new std::multiset<const std::shared_ptr<Event>, compareEvents>);
        unprocessed_queue_scheduler_map_.push_back(obj_id / num_of_schedulers);
    }
    unprocessed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);
    processed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    // Create the schedule queues and their locks
    for (unsigned int scheduler_id = 0; 
            scheduler_id < num_of_schedulers; scheduler_id++) {
        schedule_queue_.push_back(new STLLTSFQueue());
    }
    schedule_queue_lock_ = make_unique<std::mutex []>(num_of_schedulers);

    // Map worker threads to schedule queues
    for (unsigned int thread_id = 0; 
            thread_id < num_of_worker_threads; thread_id++) {
        worker_thread_scheduler_map_.push_back(thread_id % num_of_schedulers);
    }
}

void TimeWarpEventSet::insertEvent( unsigned int obj_id, 
                                    const std::shared_ptr<Event> event ) {

    unprocessed_queue_lock_[obj_id].lock();
    unprocessed_queue_[obj_id]->insert(event);

    // Insert event into schedule queue if no event has been scheduled
    if (unprocessed_queue_[obj_id]->count() == 1) {
        unsigned int scheduler_id = unprocessed_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->push(event);
        schedule_queue_lock_[scheduler_id].unlock();
    }
    unprocessed_queue_lock_[obj_id].unlock();
}

void insertEventVector( unsigned int obj_id, 
                        std::vector<const std::shared_ptr<Event>> events) {

    bool is_no_event_scheduled = false;
    unprocessed_queue_lock_[obj_id].lock();

    // Check whether schedule queue is empty
    if (unprocessed_queue_[obj_id]->empty() == true) {
        is_no_event_scheduled = true;
    }
    for (auto& event : events) {
        unprocessed_queue_[obj_id]->insert(event);
    }

    // Insert lowest event into schedule queue if no event has been scheduled
    if (is_no_event_scheduled) {
        unsigned int scheduler_id = unprocessed_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->push(
                                        *unprocessed_queue_[obj_id]->begin());
        schedule_queue_lock_[scheduler_id].unlock();
    }
    unprocessed_queue_lock_[obj_id].unlock();
}

const std::shared_ptr<Event> TimeWarpEventSet::getEvent( unsigned int thread_id ) { 

    const std::shared_ptr<Event> event = nullptr;
    schedule_queue_lock_[worker_thread_scheduler_map_[thread_id]].lock();
    unsigned int scheduler_id = worker_thread_scheduler_map_[thread_id];
    if (schedule_queue_[scheduler_id]->empty() == false) {
        event = schedule_queue_[scheduler_id]->pop();
    }
    schedule_queue_lock_[scheduler_id].unlock();
    return event;
}

const std::shared_ptr<Event> readLowestEventFromObj( unsigned int obj_id ) {

    const std::shared_ptr<Event> event = nullptr;
    unprocessed_queue_lock_[obj_id].lock();
    if (unprocessed_queue_[obj_id]->empty() == false) {
        event = *unprocessed_queue_[obj_id]->begin();
    }
    unprocessed_queue_lock_[obj_id].unlock();
    return event;
}

//TODO: APIs not ready
bool TimeWarpEventSet::handleAntiMessage ( 
                                    unsigned int obj_id, 
                                    const std::unique_ptr<Event> cancel_event ) { 

}

void TimeWarpEventSet::fossilCollect ( 
                                    SimulationObject* object, 
                                    unsigned int timestamp, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::fossilCollect ( 
                                    const std::unique_ptr<Event> event, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::rollback (   SimulationObject* object, 
                                    unsigned int rollback_time, 
                                    unsigned int thread_id ) {

}

} // namespace warped

