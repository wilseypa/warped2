#include <algorithm>
#include <cassert>
#include <string>

#include "TimeWarpEventSet.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpEventSet::initialize (unsigned int num_of_objects, 
                                   unsigned int num_of_schedulers,
                                   unsigned int num_of_worker_threads) {

    num_of_objects_ = num_of_objects;
    num_of_schedulers_ = num_of_schedulers;

    /* Create the input and processed queues and their locks.
       Also create the input queue-scheduler map and scheduled event pointer. */
    input_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    for (unsigned int obj_id = 0; obj_id < num_of_objects; obj_id++) {
        input_queue_.push_back(
            make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
        processed_queue_.push_back(
            make_unique<std::vector<std::shared_ptr<Event>>>());
        scheduled_event_pointer_.push_back(0);
        input_queue_scheduler_map_.push_back(obj_id % num_of_schedulers);
    }

    /* Create the schedule queues and their locks. */
    for (unsigned int scheduler_id = 0; 
            scheduler_id < num_of_schedulers; scheduler_id++) {
        schedule_queue_.push_back(
            make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
    }
    schedule_queue_lock_ = make_unique<std::mutex []>(num_of_schedulers);

    /* Map worker threads to schedule queues. */
    for (unsigned int thread_id = 0; 
            thread_id < num_of_worker_threads; thread_id++) {
        worker_thread_scheduler_map_.push_back(thread_id % num_of_schedulers);
    }
}

void TimeWarpEventSet::acquireInputQueueLock (unsigned int obj_id) {

    input_queue_lock_[obj_id].lock();
}

void TimeWarpEventSet::releaseInputQueueLock (unsigned int obj_id) {

    input_queue_lock_[obj_id].unlock();
}

void TimeWarpEventSet::insertEvent (unsigned int obj_id, std::shared_ptr<Event> event) {

    input_queue_[obj_id]->insert(event);
    unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
    // If no event is currently scheduled (all current events processed)
    if (!scheduled_event_pointer_[obj_id]) {
        assert(input_queue_[obj_id]->size() == 1);
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(event);
        schedule_queue_lock_[scheduler_id].unlock();
        scheduled_event_pointer_[obj_id] = event;

    } else { // Scheduled event present
        // Try to schedule the lowest unprocessed event
        auto smallest_event = *input_queue_[obj_id]->begin();
        if (smallest_event != scheduled_event_pointer_[obj_id]) {
            schedule_queue_lock_[scheduler_id].lock();
            if (schedule_queue_[scheduler_id]->erase(scheduled_event_pointer_[obj_id])) {
                schedule_queue_[scheduler_id]->insert(smallest_event);
                scheduled_event_pointer_[obj_id] = smallest_event;
            }
            schedule_queue_lock_[scheduler_id].unlock();
        }
    }
}

std::shared_ptr<Event> TimeWarpEventSet::getEvent (unsigned int thread_id) { 

    unsigned int scheduler_id = worker_thread_scheduler_map_[thread_id];
    schedule_queue_lock_[scheduler_id].lock();
    auto event_iterator = schedule_queue_[scheduler_id]->begin();
    auto event = (event_iterator != schedule_queue_[scheduler_id]->end()) ? 
                                                    *event_iterator : nullptr;
    if (event) {
        schedule_queue_[scheduler_id]->erase(event_iterator);
    }
    schedule_queue_lock_[scheduler_id].unlock();
    return event;
}

std::shared_ptr<Event> TimeWarpEventSet::lastProcessedEvent (unsigned int obj_id) {

    return ((processed_queue_[obj_id]->size()) ? processed_queue_[obj_id]->back() : 0);
}

void TimeWarpEventSet::rollback (unsigned int obj_id, std::shared_ptr<Event> straggler_event) {

    while (processed_queue_[obj_id]->size()){
        auto event = *processed_queue_[obj_id]->rbegin();
        assert(event);
        if (*event < *straggler_event) {
            break;
        } else {
            processed_queue_[obj_id]->pop_back();
            input_queue_[obj_id]->insert(event);
        }
    }
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>> TimeWarpEventSet::getEventsForCoastForward (
        unsigned int obj_id, std::shared_ptr<Event> straggler_event, 
                                std::shared_ptr<Event> restored_state_event) {

    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();
    auto event_riterator = processed_queue_[obj_id]->rbegin();
    while (event_riterator != processed_queue_[obj_id]->rend()) {
        assert(*event_riterator);
        assert(**event_riterator < *straggler_event);
        if (**event_riterator <= *restored_state_event) {
            break;
        } else {
            events->push_back(*event_riterator);
            event_riterator++;
        }
    }
    return (std::move(events));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    if (input_queue_[obj_id]->begin() != input_queue_[obj_id]->end()) {
        scheduled_event_pointer_[obj_id] = *input_queue_[obj_id]->begin();
        unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
        schedule_queue_lock_[scheduler_id].unlock();
    }
}

void TimeWarpEventSet::replenishScheduler (unsigned int obj_id) {

    // If an event was previously scheduled
    if (scheduled_event_pointer_[obj_id]) {
        if (!input_queue_[obj_id]->erase(scheduled_event_pointer_[obj_id])) {
            assert(0);
        }
        processed_queue_[obj_id]->push_back(scheduled_event_pointer_[obj_id]);
        if (input_queue_[obj_id]->begin() != input_queue_[obj_id]->end()) {
            scheduled_event_pointer_[obj_id] = *input_queue_[obj_id]->begin();
            unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
            schedule_queue_lock_[scheduler_id].lock();
            schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
            schedule_queue_lock_[scheduler_id].unlock();
        }
    } else {
        assert(0);
    }
}

void TimeWarpEventSet::cancelEvent (unsigned int obj_id, std::shared_ptr<Event> cancel_event) {

    auto neg_iterator = input_queue_[obj_id]->find(cancel_event);
    assert(neg_iterator != input_queue_[obj_id]->end());
    auto pos_iterator = std::next(neg_iterator);
    assert(**pos_iterator == **neg_iterator);
    assert((*pos_iterator)->event_type_ == EventType::POSITIVE);
    input_queue_[obj_id]->erase(neg_iterator);
    input_queue_[obj_id]->erase(pos_iterator);
}

void TimeWarpEventSet::fossilCollect (unsigned int fossil_collect_time, unsigned int obj_id) {

    auto event_iterator = processed_queue_[obj_id]->begin();
    while (event_iterator != processed_queue_[obj_id]->end()) {
        if ((*event_iterator)->timestamp() >= fossil_collect_time) {
            break;
        }
        processed_queue_[obj_id]->erase(event_iterator);
        event_iterator = processed_queue_[obj_id]->begin();
    }
}

} // namespace warped

