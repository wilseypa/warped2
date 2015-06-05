#include <algorithm>
#include <cassert>
#include <string>

#include "TimeWarpEventSet.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpEventSet::initialize (unsigned int num_of_objects, 
                                   unsigned int num_of_schedulers,
                                   bool is_lp_migration_on,
                                   unsigned int num_of_worker_threads) {

    num_of_objects_     = num_of_objects;
    num_of_schedulers_  = num_of_schedulers;
    is_lp_migration_on_ = is_lp_migration_on;

    /* Create the input and processed queues and their locks.
       Also create the input queue-scheduler map and scheduled event pointer. */
    input_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    for (unsigned int obj_id = 0; obj_id < num_of_objects; obj_id++) {
        input_queue_.push_back(
            make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
        processed_queue_.push_back(
            make_unique<std::vector<std::shared_ptr<Event>>>());
        scheduled_event_pointer_.push_back(nullptr);
        input_queue_scheduler_map_.push_back(obj_id % num_of_schedulers);
    }

    /* Create the schedule queues and their locks. */
    for (unsigned int scheduler_id = 0; scheduler_id < num_of_schedulers; scheduler_id++) {
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

/*
 *  NOTE: caller must always have the input queue lock for the object with id obj_id
 *
 *  NOTE: scheduled_event_pointer is also protected by the input queue lock
 */
void TimeWarpEventSet::insertEvent (unsigned int obj_id, std::shared_ptr<Event> event) {

    // Always insert event into input queue
    input_queue_[obj_id]->insert(event);

    unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];

    if (scheduled_event_pointer_[obj_id] == nullptr) {
        // If no event is currently scheduled. This can only happen if the thread that handles events
        // for object with id obj_id has determined that there are no more events left in it's input
        // queue

        assert(input_queue_[obj_id]->size() == 1);

        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(event);
        schedule_queue_lock_[scheduler_id].unlock();
        scheduled_event_pointer_[obj_id] = event;

    } else {
        auto smallest_event = *input_queue_[obj_id]->begin();

        if (smallest_event != scheduled_event_pointer_[obj_id]) {
            // If the pointer comparison of the smallest event does not match scheduled event, well
            // that means we should update the schedule queue...

            schedule_queue_lock_[scheduler_id].lock();
            if (auto num_erased = schedule_queue_[scheduler_id]->erase(scheduled_event_pointer_[obj_id])) {
                // ...but only if the event was successfully erased from the schedule queue. If it is
                // not then the event is already being processed and a rollback will have to occur.

                assert(num_erased == 1);
                unused(num_erased);

                schedule_queue_[scheduler_id]->insert(smallest_event);
                scheduled_event_pointer_[obj_id] = smallest_event;
            }
            schedule_queue_lock_[scheduler_id].unlock();
        }
    }
}

/*
 *  NOTE: caller must always have the input queue lock for the object with id obj_id
 */
std::shared_ptr<Event> TimeWarpEventSet::getEvent (unsigned int thread_id) {

    unsigned int scheduler_id = worker_thread_scheduler_map_[thread_id];

    schedule_queue_lock_[scheduler_id].lock();

    auto event_iterator = schedule_queue_[scheduler_id]->begin();
    auto event = (event_iterator != schedule_queue_[scheduler_id]->end()) ?
                    *event_iterator : nullptr;
    if (event != nullptr) {
        schedule_queue_[scheduler_id]->erase(event_iterator);
    }

    // NOTE: scheduled_event_pointer is not changed here so that other threads will not schedule new
    // events and this thread can move events into processed queue and update schedule queue correctly.

    // NOTE: Event also remains in input queue until processing done. If this a a negative event
    // then, a rollback will bring the processed positive event back to input queue and they will
    // be cancelled.

    schedule_queue_lock_[scheduler_id].unlock();

    return event;
}

/*
 *  NOTE: caller must have the input queue lock for the object with id obj_id
 */
std::shared_ptr<Event> TimeWarpEventSet::lastProcessedEvent (unsigned int obj_id) {

    return ((processed_queue_[obj_id]->size()) ? processed_queue_[obj_id]->back() : nullptr);
}

/*
 *  NOTE: caller must have the input queue lock for the object with id obj_id
 */
void TimeWarpEventSet::rollback (unsigned int obj_id, std::shared_ptr<Event> straggler_event) {

    // Every event GREATER OR EQUAL to straggler event must remove from the processed queue and
    // reinserted back into input queue.
    // EQUAL will ensure that a negative message will properly be cancelled out.

    while (!processed_queue_[obj_id]->empty()){

        auto event = processed_queue_[obj_id]->back(); // Starting from largest event
        assert(event);

        if (*event < *straggler_event) {
            // Processed events are always in order, so we are safe to break when we reach a lesser
            // event.
            break;
        } else {
            processed_queue_[obj_id]->pop_back();
            input_queue_[obj_id]->insert(event);
        }
    }
}

/*
 *  NOTE: caller must have the input queue lock for the object with id obj_id
 */
std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
    TimeWarpEventSet::getEventsForCoastForward (
                                unsigned int obj_id, 
                                std::shared_ptr<Event> straggler_event, 
                                std::shared_ptr<Event> restored_state_event) {

    // To avoid error if asserts are disabled
    unused(straggler_event);

    // Restored state event is the last event to contribute to the current state of the object.
    // All events GREATER THAN this event but LESS THAN the straggler event must be "coast forwarded"
    // so that the state remains consistent.
    //
    // It is assumed that all processed events GREATER THAN OR EQUAL to the straggler event have
    // been moved from the processed queue to the input queue with a call to rollback().
    //
    // All coast forwared events remain in the processed queue.

    // Create empty vector
    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();

    auto event_riterator = processed_queue_[obj_id]->rbegin();  // Starting with largest event

    while (event_riterator != processed_queue_[obj_id]->rend()) {

        assert(*event_riterator);
        assert(**event_riterator < *straggler_event);

        if (**event_riterator <= *restored_state_event) {
            // Processed events are always in order so it's safe to stop when we reach an event
            // that is less than or equal to restored state event.
            break;
        } else {
            // Events are in order of LARGEST to SMALLEST
            events->push_back(*event_riterator);
            event_riterator++;
        }
    }

    return (std::move(events));
}

/*
 *  NOTE: call must always have input queue lock for the object which corresponds to obj_id
 *
 *  NOTE: This is called in the case of an negative message and no event is processed.
 *
 *  NOTE: This can only be called by the thread that handles events for the object with id obj_id
 *
 */
void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    // Just simply add pointer to next event into the scheduler if input queue is not empty
    // for the given object, otherwise set to nullptr
    if (!input_queue_[obj_id]->empty()) {
        scheduled_event_pointer_[obj_id] = *input_queue_[obj_id]->begin();
        unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
        schedule_queue_lock_[scheduler_id].unlock();
    } else {
        scheduled_event_pointer_[obj_id] = nullptr;
    }
}

/*
 *  NOTE: This can only be called by the thread that handles event for the object with id obj_id
 *
 *  NOTE: caller must always have the input queue lock for the object which corresponds to obj_id
 *
 *  NOTE: the scheduled_event_pointer is also protected by input queue lock
 */
void TimeWarpEventSet::replenishScheduler (unsigned int obj_id) {

    // Something is completely wrong if there is no scheduled event because we obviously just
    // processed an event that was scheduled.
    assert(scheduled_event_pointer_[obj_id]);

    // Move the just processed event to the processed queue
    auto num_erased = input_queue_[obj_id]->erase(scheduled_event_pointer_[obj_id]);
    assert(num_erased == 1);
    unused(num_erased);
    processed_queue_[obj_id]->push_back(scheduled_event_pointer_[obj_id]);

    // Map the object to the next schedule queue (cyclic order)
    // This is supposed to balance the load across all the schedule queues
    // Input queue lock is sufficient to ensure consistency
    unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
    if (is_lp_migration_on_) {
        scheduler_id = (scheduler_id + 1) % num_of_schedulers_;
        input_queue_scheduler_map_[obj_id] = scheduler_id;
    }

    // Update scheduler with new event for the object the previous event was executed for
    // NOTE: A pointer to the scheduled event will remain in the input queue
    if (!input_queue_[obj_id]->empty()) {
        scheduled_event_pointer_[obj_id] = *input_queue_[obj_id]->begin();
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
        schedule_queue_lock_[scheduler_id].unlock();
    } else {
        scheduled_event_pointer_[obj_id] = nullptr;
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

// For debugging
void TimeWarpEventSet::printEvent(std::shared_ptr<Event> event) {
    std::cout << "\tSender:     " << event->sender_name_                  << "\n"
              << "\tReceiver:   " << event->receiverName()                << "\n"
              << "\tSend time:  " << event->send_time_                    << "\n"
              << "\tRecv time:  " << event->timestamp()                   << "\n"
              << "\tCounter:    " << event->counter_                      << "\n"
              << "\tType:       " << (unsigned int)event->event_type_     << "\n";
}

unsigned int TimeWarpEventSet::fossilCollect (unsigned int fossil_collect_time, unsigned int obj_id) {

    unsigned int count = 0;

    auto event_iterator = processed_queue_[obj_id]->begin();
    while (event_iterator != processed_queue_[obj_id]->end()) {
        if ((*event_iterator)->timestamp() >= fossil_collect_time) {
            break;
        }
        processed_queue_[obj_id]->erase(event_iterator);
        count++;
        event_iterator = processed_queue_[obj_id]->begin();
    }

    return count;
}

} // namespace warped

