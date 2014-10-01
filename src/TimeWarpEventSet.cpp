#include "TimeWarpEventSet.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpEventSet::initialize (unsigned int num_of_objects, 
                                   unsigned int num_of_schedulers,
                                   unsigned int num_of_worker_threads) {

    num_of_objects_ = num_of_objects;
    num_of_schedulers_ = num_of_schedulers;

    /* Create the unprocessed and processed queues and their locks.
       Also create the uprocessed queue to schedule queue map. */

    is_unprocessed_queue_empty_ = make_unique<bool []>(num_of_objects); 
    unprocessed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);
    processed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    for (unsigned int obj_id = 0; obj_id < num_of_objects; obj_id++) {
        unprocessed_queue_.push_back(
            make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
        processed_queue_.push_back(
            make_unique<std::vector<std::shared_ptr<Event>>>());
        coast_forward_queue_.push_back(
            make_unique<std::vector<std::shared_ptr<Event>>>());
        unprocessed_queue_scheduler_map_.push_back(obj_id % num_of_schedulers);

        // Initialization for the boolean array
        is_unprocessed_queue_empty_[obj_id] = false;
    }

    // Create the schedule queues and their locks
    for (unsigned int scheduler_id = 0; 
            scheduler_id < num_of_schedulers; scheduler_id++) {
        schedule_queue_.push_back(make_unique<STLLTSFQueue>());
    }
    schedule_queue_lock_ = make_unique<std::mutex []>(num_of_schedulers);

    // Map worker threads to schedule queues
    for (unsigned int thread_id = 0; 
            thread_id < num_of_worker_threads; thread_id++) {
        worker_thread_scheduler_map_.push_back(thread_id % num_of_schedulers);
    }
}

void TimeWarpEventSet::insertEvent (unsigned int obj_id, 
                                    std::shared_ptr<Event> event) {

    unprocessed_queue_lock_[obj_id].lock();
    if (event->event_type_ == EventType::NEGATIVE) {
        bool found_event = false;
        for (auto event_iterator = unprocessed_queue_[obj_id]->begin();
                    event_iterator != unprocessed_queue_[obj_id]->end() && 
                                (*event_iterator)->timestamp() <= event->timestamp(); 
                    event_iterator++) {
            if (*event == **event_iterator) {
                unprocessed_queue_[obj_id]->erase(event_iterator);
                found_event = true;
                break;
            }
        }
        if (!found_event) {
            unprocessed_queue_[obj_id]->insert(event);
        }
    } else {
        if (unprocessed_queue_[obj_id]->empty() && 
                is_unprocessed_queue_empty_[obj_id]) {
            unsigned int scheduler_id = unprocessed_queue_scheduler_map_[obj_id];
            schedule_queue_lock_[scheduler_id].lock();
            schedule_queue_[scheduler_id]->push(event);
            schedule_queue_lock_[scheduler_id].unlock();
            is_unprocessed_queue_empty_[obj_id] = false;
        } else {
            unprocessed_queue_[obj_id]->insert(event);
        }
    }
    unprocessed_queue_lock_[obj_id].unlock();
}

std::shared_ptr<Event> TimeWarpEventSet::getEvent (unsigned int thread_id) { 

    std::shared_ptr<Event> event = nullptr;
    unsigned int scheduler_id = worker_thread_scheduler_map_[thread_id];
    schedule_queue_lock_[scheduler_id].lock();
    if (schedule_queue_[scheduler_id]->empty() == false) {
        event = schedule_queue_[scheduler_id]->pop();
    }
    schedule_queue_lock_[scheduler_id].unlock();
    return event;
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
        TimeWarpEventSet::getEventsForCoastForward (unsigned int obj_id) {

    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();
    processed_queue_lock_[obj_id].lock();
    auto events_coast_forward = std::move(coast_forward_queue_[obj_id]);
    coast_forward_queue_[obj_id] = std::move(events);
    processed_queue_lock_[obj_id].unlock();
    return (std::move(events_coast_forward));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    unprocessed_queue_lock_[obj_id].lock();
    if (!unprocessed_queue_[obj_id]->empty()) {
        std::shared_ptr<Event> event = *unprocessed_queue_[obj_id]->begin();
        unsigned int scheduler_id = unprocessed_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->push(event);
        schedule_queue_lock_[scheduler_id].unlock();
        unprocessed_queue_[obj_id]->erase(event);
    } else {
        is_unprocessed_queue_empty_[obj_id] = true;
    }
    unprocessed_queue_lock_[obj_id].unlock();
}

void TimeWarpEventSet::replenishScheduler (unsigned int obj_id, std::shared_ptr<Event> old_event) {

    // Move old event from unprocessed queue to processed queue. 
    // Note: Old event might not be the smallest event in unprocessed queue.
    processed_queue_lock_[obj_id].lock();
    processed_queue_[obj_id]->push_back(old_event);
    processed_queue_lock_[obj_id].unlock();

    startScheduling(obj_id);
}

void TimeWarpEventSet::fossilCollectAll (unsigned int fossil_collect_time) {

    for (unsigned int obj_id = 0; obj_id < num_of_objects_; obj_id++) {
        processed_queue_lock_[obj_id].lock();
        while (processed_queue_[obj_id]->begin() != processed_queue_[obj_id]->end() ) {
            auto event_iterator = processed_queue_[obj_id]->begin();
            if ((*event_iterator)->timestamp() >= fossil_collect_time) break;
            processed_queue_[obj_id]->erase(event_iterator);
        }
        processed_queue_lock_[obj_id].unlock();
    }
}

void TimeWarpEventSet::rollback (unsigned int obj_id, unsigned int rollback_time) {

    processed_queue_lock_[obj_id].lock();
    unsigned int processed_queue_len = processed_queue_[obj_id]->size();
    while ((processed_queue_len > 0) && 
           ((*processed_queue_[obj_id])[processed_queue_len-1]->timestamp() >= rollback_time)) {
        coast_forward_queue_[obj_id]->insert(coast_forward_queue_[obj_id]->begin(), 
                                                (*processed_queue_[obj_id])[processed_queue_len-1]);
        processed_queue_[obj_id]->pop_back();
        processed_queue_len--;
    }
    processed_queue_lock_[obj_id].unlock();
}

} // namespace warped

