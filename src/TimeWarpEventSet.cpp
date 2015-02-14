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

    /* Create the input queues and their locks.
       Also create the input queue to schedule queue map. */
    input_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    for (unsigned int obj_id = 0; obj_id < num_of_objects; obj_id++) {
        input_queue_.push_back(
            make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
        track_processed_event_.push_back(
            make_unique<std::unordered_map<std::shared_ptr<Event>, bool>>());
        straggler_event_pointer_.push_back(0);
        scheduled_event_pointer_.push_back(0);
        input_queue_scheduler_map_.push_back(obj_id % num_of_schedulers);
    }

    /* Create the schedule queues and their locks. */
    for (unsigned int scheduler_id = 0; 
            scheduler_id < num_of_schedulers; scheduler_id++) {
        schedule_queue_.push_back(make_unique<
                            std::multiset<std::shared_ptr<Event>, compareEvents>>());
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

    bool causal_order_ok = true;
    auto event_iterator = input_queue_[obj_id]->insert(event);
    (*track_processed_event_[obj_id])[event] = false;

    // If event is negative
    // Assumption: Positive event arrives earlier than its negative counterpart
    if (event->event_type_ == EventType::NEGATIVE) {
        auto next_iterator = std::next(event_iterator);

        // If next event is the positive counterpart
        if ((next_iterator != input_queue_[obj_id]->end()) &&
                (**next_iterator == *event) && 
                ((*next_iterator)->event_type_ == EventType::POSITIVE)) {

            // If no event is currently scheduled (all current events processed)
            if (!scheduled_event_pointer_[obj_id]) {
                scheduled_event_pointer_[obj_id] = event;
                causal_order_ok = false;

            } else { // Scheduled event present
                // If positive counterpart is currently scheduled
                if (scheduled_event_pointer_[obj_id] == *next_iterator) {
                    causal_order_ok = false;

                } else if (**next_iterator < *scheduled_event_pointer_[obj_id]) { 
                    // If negative event is supposed to cause a rollback
                    causal_order_ok = false;

                } else { // It is ok to delete the negative event
                    (*track_processed_event_[obj_id]).erase(*event_iterator);
                    (*track_processed_event_[obj_id]).erase(*next_iterator);
                    input_queue_[obj_id]->erase(event_iterator);
                    input_queue_[obj_id]->erase(next_iterator);
                }
            }
        } else { // If no positive counterpart found
            assert(0);
        }
    } else { // Positive event
        // If no event is currently scheduled (all current events processed)
        if (!scheduled_event_pointer_[obj_id]) {
            scheduled_event_pointer_[obj_id] = event;
            // Initial event should not be classified as a straggler
            if ((input_queue_[obj_id]->size() > 1) && 
                        (event != *input_queue_[obj_id]->rbegin())) {
                causal_order_ok = false;
            }
        } else { // Scheduled event present
            // If event is smaller than the one scheduled
            if (*event < *scheduled_event_pointer_[obj_id]) {
                causal_order_ok = false;
            }
        }
    }

    // If inserted event is the one scheduled, insert into schedule queue
    if (scheduled_event_pointer_[obj_id] == event) {
        unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(event);
        schedule_queue_lock_[scheduler_id].unlock();
    }

    if (!causal_order_ok) {
        // If straggler already exists
        if (straggler_event_pointer_[obj_id]) {
            // If event less than existing straggler
            if ((*event < *straggler_event_pointer_[obj_id]) || 
                ((*event == *straggler_event_pointer_[obj_id]) && 
                    (event->event_type_ < straggler_event_pointer_[obj_id]->event_type_))) {
                straggler_event_pointer_[obj_id] = event;
            }
        } else { // Straggler does not exist
            straggler_event_pointer_[obj_id] = event;
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

void TimeWarpEventSet::processedEvent (unsigned int obj_id, std::shared_ptr<Event> event) {

    (*track_processed_event_[obj_id])[event] = true;
}

std::shared_ptr<Event> TimeWarpEventSet::getStragglerEvent (unsigned int obj_id) {

    return straggler_event_pointer_[obj_id];
}

void TimeWarpEventSet::resetStragglerEvent (unsigned int obj_id) {

    straggler_event_pointer_[obj_id] = 0;
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
        TimeWarpEventSet::getEventsForCoastForward (
                unsigned int obj_id, 
                std::shared_ptr<Event> straggler_event,
                std::shared_ptr<Event> restored_state_event) {

    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();

    auto straggler_iterator = input_queue_[obj_id]->find(straggler_event);
    auto restored_event_iterator = input_queue_[obj_id]->find(restored_state_event);
    assert(straggler_iterator != input_queue_[obj_id]->end());
    assert(restored_event_iterator != input_queue_[obj_id]->end());

    do {
        straggler_iterator--;
        assert(*straggler_iterator);
        if (straggler_iterator == restored_event_iterator) {
            break;
        }
        if ((*track_processed_event_[obj_id])[*straggler_iterator]) {
            events->push_back(*straggler_iterator);
        }
    } while (straggler_iterator != input_queue_[obj_id]->begin());

    return (std::move(events));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    auto straggler_event = straggler_event_pointer_[obj_id];
    if (scheduled_event_pointer_[obj_id]) {
        // If straggler exists
        if (straggler_event) {
            auto event_iterator = input_queue_[obj_id]->find(straggler_event);
            if (event_iterator == input_queue_[obj_id]->end()) {
                assert(0);
            }
            scheduled_event_pointer_[obj_id] = straggler_event;

        } else { // Causal order
            auto event_iterator = input_queue_[obj_id]->find(scheduled_event_pointer_[obj_id]);
            if (event_iterator == input_queue_[obj_id]->end()) {
                assert(0);
            }
            event_iterator++;
            scheduled_event_pointer_[obj_id] = 
                (event_iterator != input_queue_[obj_id]->end()) ? *event_iterator : 0;
        }
        if (scheduled_event_pointer_[obj_id]) {
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

    auto next_valid_iterator = std::next(neg_iterator, 2);
    scheduled_event_pointer_[obj_id] = 
        (next_valid_iterator != input_queue_[obj_id]->end()) ? *next_valid_iterator : 0;
    if (scheduled_event_pointer_[obj_id]) {
        unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
        schedule_queue_lock_[scheduler_id].unlock();
    }
    (*track_processed_event_[obj_id]).erase(*neg_iterator);
    (*track_processed_event_[obj_id]).erase(*pos_iterator);
    input_queue_[obj_id]->erase(neg_iterator);
    input_queue_[obj_id]->erase(pos_iterator);
}

void TimeWarpEventSet::fossilCollectAll (unsigned int fossil_collect_time) {

    for (unsigned int obj_id = 0; obj_id < num_of_objects_; obj_id++) {
        acquireInputQueueLock(obj_id);
        auto event_iterator = input_queue_[obj_id]->begin();
        for (; event_iterator != input_queue_[obj_id]->end(); event_iterator++) {
            if ((*event_iterator)->timestamp() >= fossil_collect_time) {
                break;
            }
        }
        if (event_iterator != input_queue_[obj_id]->begin()) {
            input_queue_[obj_id]->erase(input_queue_[obj_id]->begin(), 
                                            std::prev(event_iterator));
        }
        releaseInputQueueLock(obj_id);
    }
}

} // namespace warped

