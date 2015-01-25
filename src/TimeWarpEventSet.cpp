#include <algorithm>
#include <cassert>

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
        scheduled_event_pointer_.push_back(input_queue_[obj_id]->end());
        lowest_event_pointer_.push_back(input_queue_[obj_id]->end());
        straggler_event_pointer_.push_back(input_queue_[obj_id]->end());
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

void TimeWarpEventSet::insertEvent (unsigned int obj_id, 
                                    std::shared_ptr<Event> event) {

    input_queue_lock_[obj_id].lock();
    auto event_iterator = input_queue_[obj_id]->insert(event);
    unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];

    // Update the scheduled event pointers (if needed)
    if (scheduled_event_pointer_[obj_id] == input_queue_[obj_id]->end()) {
        scheduled_event_pointer_[obj_id] = event_iterator;
    }

    // If event is negative
    // Assumption: Positive event arrives earlier than its negative counterpart
    if (event->event_type_ == EventType::NEGATIVE) {
        auto next_iterator = std::next(event_iterator, 1);

        // If next event is the positive counterpart
        if ((next_iterator != input_queue_[obj_id]->end()) &&
                (**next_iterator == **event_iterator) && 
                ((*next_iterator)->event_type_ == EventType::POSITIVE)) {

            // If positive counterpart is currently scheduled
            if (scheduled_event_pointer_[obj_id] == next_iterator) {
                // Try to delete the event from schedule queue
                schedule_queue_lock_[scheduler_id].lock();
                // If event deletion not successful
                if (!schedule_queue_[scheduler_id]->erase(*next_iterator)) {
                    if ((lowest_event_pointer_[obj_id] == input_queue_[obj_id]->end()) || 
                            (**event_iterator < **lowest_event_pointer_[obj_id])) {
                        lowest_event_pointer_[obj_id] = event_iterator;
                    }
                } else { // Event deleted
                    scheduled_event_pointer_[obj_id] = std::next(next_iterator, 1);
                    if (scheduled_event_pointer_[obj_id] != input_queue_[obj_id]->end()) {
                        schedule_queue_[scheduler_id]->insert(*scheduled_event_pointer_[obj_id]);
                    }
                    input_queue_[obj_id]->erase(event_iterator);
                    input_queue_[obj_id]->erase(next_iterator);
                }
                schedule_queue_lock_[scheduler_id].unlock();

            } else if (**next_iterator < **scheduled_event_pointer_[obj_id]) { 
                // If negative event is supposed to cause a rollback
                if ((lowest_event_pointer_[obj_id] == input_queue_[obj_id]->end()) || 
                            (**next_iterator < **lowest_event_pointer_[obj_id])) {
                    lowest_event_pointer_[obj_id] = event_iterator;
                }
            } else { // It is ok to delete the negative event
                lowest_event_pointer_[obj_id] = scheduled_event_pointer_[obj_id];
                //(*event_iterator).reset();
                //(*next_iterator).reset();
                input_queue_[obj_id]->erase(event_iterator);
                input_queue_[obj_id]->erase(next_iterator);
            }
        } else { // If no positive counterpart found
            assert(0);
        }
    } else { // Positive event
        // If event is smaller than the one scheduled
        if (**event_iterator < **scheduled_event_pointer_[obj_id]) {
            // If event lies just before the scheduled event
            if (scheduled_event_pointer_[obj_id] == std::next(event_iterator, 1)) {
                // Try to delete the event from schedule queue
                schedule_queue_lock_[scheduler_id].lock();
                // If deletion from schedule queue fails
                if (!schedule_queue_[scheduler_id]->erase(*scheduled_event_pointer_[obj_id])) {
                    if ((lowest_event_pointer_[obj_id] == input_queue_[obj_id]->end()) || 
                            (**event_iterator < **lowest_event_pointer_[obj_id])) {
                        lowest_event_pointer_[obj_id] = event_iterator;
                    }
                } else { // Event deleted
                    scheduled_event_pointer_[obj_id] = event_iterator;
                    schedule_queue_[scheduler_id]->insert(event);
                }
                schedule_queue_lock_[scheduler_id].unlock();

            } else { // When event needs to be rolled back
                if ((lowest_event_pointer_[obj_id] == input_queue_[obj_id]->end()) || 
                            (**event_iterator < **lowest_event_pointer_[obj_id])) {
                    lowest_event_pointer_[obj_id] = event_iterator;
                }
            }
        }
    }
    input_queue_lock_[obj_id].unlock();
}

std::shared_ptr<Event> TimeWarpEventSet::getEvent (unsigned int thread_id) { 

    unsigned int scheduler_id = worker_thread_scheduler_map_[thread_id];
    schedule_queue_lock_[scheduler_id].lock();
    auto event_iterator = schedule_queue_[scheduler_id]->begin();
    auto event = (event_iterator != schedule_queue_[scheduler_id]->end()) ? 
                                                    *event_iterator : nullptr;
    schedule_queue_[scheduler_id]->erase(event_iterator);
    schedule_queue_lock_[scheduler_id].unlock();
    return event;
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
        TimeWarpEventSet::getEventsForCoastForward (unsigned int obj_id) {

    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();
    input_queue_lock_[obj_id].lock();
    for (auto event_iterator = input_queue_[obj_id]->begin(); ; event_iterator++) {
        events->push_back(*event_iterator);
        if (event_iterator == straggler_event_pointer_[obj_id]) {
            break;
        }
    }
    input_queue_lock_[obj_id].unlock();
    return (std::move(events));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    input_queue_lock_[obj_id].lock();
    if (scheduled_event_pointer_[obj_id] != input_queue_[obj_id]->end()) {
        scheduled_event_pointer_[obj_id]++;
        if (scheduled_event_pointer_[obj_id] != input_queue_[obj_id]->end()) {
            unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
            schedule_queue_lock_[scheduler_id].lock();
            schedule_queue_[scheduler_id]->insert(*scheduled_event_pointer_[obj_id]);
            schedule_queue_lock_[scheduler_id].unlock();
        }
    }
    input_queue_lock_[obj_id].unlock();
}

void TimeWarpEventSet::fossilCollectAll (unsigned int fossil_collect_time) {

    for (unsigned int obj_id = 0; obj_id < num_of_objects_; obj_id++) {
        input_queue_lock_[obj_id].lock();
        auto event_iterator = input_queue_[obj_id]->begin();
        for (; event_iterator != input_queue_[obj_id]->end(); event_iterator++) {
            if ((*event_iterator)->timestamp() >= fossil_collect_time) {
                break;
            }
        }
        input_queue_[obj_id]->erase(input_queue_[obj_id]->begin(), 
                                        std::prev(event_iterator, 1));
        input_queue_lock_[obj_id].unlock();
    }
}

void TimeWarpEventSet::rollback (unsigned int obj_id, std::shared_ptr<Event> straggler_event) {

    input_queue_lock_[obj_id].lock();
    auto straggler_iterator = input_queue_[obj_id]->find(straggler_event);
    if (straggler_iterator == input_queue_[obj_id]->end()) {
        assert(0);
    }
    straggler_event_pointer_[obj_id] = straggler_iterator;
    input_queue_lock_[obj_id].unlock();
}

} // namespace warped

