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

bool TimeWarpEventSet::insertEvent (unsigned int obj_id, 
                                    std::shared_ptr<Event> event) {

    bool causal_order_ok = true;

    input_queue_lock_[obj_id].lock();
    /*std::cout << "ins - " << event << " , r = " << event->timestamp() 
              << " , send = " << event->sender_name_ << " , recv = " 
              << event->receiverName() << " , type = " 
              << ((event->event_type_ == EventType::NEGATIVE) ? 0 : 1) 
              << " , s = " << event->send_time_ << " , c = " 
              << event->counter_ << std::endl;*/

    auto event_iterator = input_queue_[obj_id]->insert(event);

    // If event is negative
    // Assumption: Positive event arrives earlier than its negative counterpart
    if (event->event_type_ == EventType::NEGATIVE) {
        auto next_iterator = std::next(event_iterator);

        // If next event is the positive counterpart
        if ((next_iterator != input_queue_[obj_id]->end()) &&
                (**next_iterator == **event_iterator) && 
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
                (*event < **input_queue_[obj_id]->rbegin())) {
                causal_order_ok = false;
            }
        } else { // Scheduled event present
            // If event is smaller than the one scheduled
            if (**event_iterator < *scheduled_event_pointer_[obj_id]) {
                causal_order_ok = false;
            }
        }
    }

    // If inserted event is the one scheduled, insert into schedule queue
    if (scheduled_event_pointer_[obj_id] == event) {
        unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
        schedule_queue_lock_[scheduler_id].unlock();
    }

    input_queue_lock_[obj_id].unlock();
    return causal_order_ok;
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
        TimeWarpEventSet::getEventsForCoastForward (
                unsigned int obj_id, 
                std::shared_ptr<Event> straggler_event,
                std::shared_ptr<Event> restored_state_event) {

    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();

    input_queue_lock_[obj_id].lock();
    auto straggler_iterator = input_queue_[obj_id]->find(straggler_event);
    assert(straggler_iterator != input_queue_[obj_id]->end());

    for (auto event_iterator = std::prev(straggler_iterator, 1); ; event_iterator--) {
        assert(*event_iterator);
        if (**event_iterator <= *restored_state_event) {
            break;
        }
        events->push_back(*event_iterator);
        if (event_iterator == input_queue_[obj_id]->begin()) {
            break;
        }
    }

    input_queue_lock_[obj_id].unlock();

    return (std::move(events));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id, 
                                        std::shared_ptr<Event> processed_event) {

    input_queue_lock_[obj_id].lock();
    if (scheduled_event_pointer_[obj_id]) {
        auto event_iterator = input_queue_[obj_id]->find(processed_event);
        if (event_iterator == input_queue_[obj_id]->end()) {
            assert(0);
        }
        if (scheduled_event_pointer_[obj_id] == processed_event) {
            event_iterator++;
            scheduled_event_pointer_[obj_id] = 
                (event_iterator != input_queue_[obj_id]->end()) ? *event_iterator : 0;
        } else { // straggler event
            scheduled_event_pointer_[obj_id] = processed_event;
        }
        if (scheduled_event_pointer_[obj_id]) {
            unsigned int scheduler_id = input_queue_scheduler_map_[obj_id];
            schedule_queue_lock_[scheduler_id].lock();
            schedule_queue_[scheduler_id]->insert(scheduled_event_pointer_[obj_id]);
            schedule_queue_lock_[scheduler_id].unlock();
        }
    } else {
        //assert(0);
    }
    input_queue_lock_[obj_id].unlock();
}

void TimeWarpEventSet::cancelEvent (unsigned int obj_id, std::shared_ptr<Event> cancel_event) {

    input_queue_lock_[obj_id].lock();
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
    input_queue_[obj_id]->erase(neg_iterator);
    input_queue_[obj_id]->erase(pos_iterator);
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

} // namespace warped

