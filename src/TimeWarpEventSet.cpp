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
                (*event_iterator).reset();
                (*next_iterator).reset();
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
    processed_queue_lock_[obj_id].lock();
    auto events_coast_forward = std::move(coast_forward_queue_[obj_id]);
    coast_forward_queue_[obj_id] = std::move(events);
    processed_queue_lock_[obj_id].unlock();
    return (std::move(events_coast_forward));
}

void TimeWarpEventSet::startScheduling (unsigned int obj_id) {

    input_queue_lock_[obj_id].lock();
    if (!unprocessed_queue_[obj_id]->empty()) {
        std::shared_ptr<Event> event = *unprocessed_queue_[obj_id]->begin();

        unsigned int scheduler_id = unprocessed_queue_scheduler_map_[obj_id];
        schedule_queue_lock_[scheduler_id].lock();
        schedule_queue_[scheduler_id]->push(event);
        assert(schedule_queue_[scheduler_id]->size() <= std::ceil(num_of_objects_/num_of_schedulers_));
        schedule_queue_lock_[scheduler_id].unlock();
        event_scheduled_from_obj_[obj_id] = event;
        unprocessed_queue_[obj_id]->erase(event);
    } else {
        event_scheduled_from_obj_[obj_id] = nullptr;
    }
    unprocessed_queue_lock_[obj_id].unlock();
}

void TimeWarpEventSet::coastForwardedEvent(unsigned int obj_id, std::shared_ptr<Event> event) {

    processed_queue_lock_[obj_id].lock();
    processed_queue_[obj_id]->push_back(event);
    processed_queue_lock_[obj_id].unlock();
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
        for (auto event_iterator = processed_queue_[obj_id]->begin(); 
                event_iterator != processed_queue_[obj_id]->end();) {
            if ((*event_iterator)->timestamp() >= fossil_collect_time) {
                event_iterator++;
            } else {
                event_iterator->reset();
                event_iterator = processed_queue_[obj_id]->erase(event_iterator);
            }
        }
        processed_queue_lock_[obj_id].unlock();
    }
}

void TimeWarpEventSet::rollback (unsigned int obj_id, unsigned int restored_time, 
                                            std::shared_ptr<Event> straggler_event) {

    processed_queue_lock_[obj_id].lock();
    unsigned int processed_queue_len = processed_queue_[obj_id]->size();
    while (processed_queue_len > 0) {
        auto processed_event = (*processed_queue_[obj_id])[processed_queue_len-1];
        assert(processed_event != nullptr);
        if (processed_event->timestamp() >= restored_time) {
            if ((*processed_event < *straggler_event) || (*processed_event == *straggler_event)) {
                coast_forward_queue_[obj_id]->insert(
                        coast_forward_queue_[obj_id]->begin(), processed_event);
                processed_queue_[obj_id]->pop_back();

            } else {
                bool found_event = false;
                unprocessed_queue_lock_[obj_id].lock();
                for (auto event : *unprocessed_queue_[obj_id]) {
                    assert(event != nullptr);
                    if (*processed_event == *event) {
                        unprocessed_queue_[obj_id]->erase(processed_event);
                        processed_event.reset();
                        event.reset();
                        found_event = true;
                    }
                }
                if (!found_event) {
                    unprocessed_queue_[obj_id]->insert(processed_event);
                }
                unprocessed_queue_lock_[obj_id].unlock();
            }
        }
        processed_queue_len--;
    }
    processed_queue_lock_[obj_id].unlock();

    straggler_flags_lock_.lock();
    continuous_straggler_flags_[obj_id]--;
    continuous_straggler_cnt_--;
    if (gvt_flag_ && (gvt_straggler_flags_[obj_id] > 0)) {
        gvt_straggler_flags_[obj_id]--;
        gvt_straggler_cnt_--;
        if (gvt_straggler_cnt_ == 0) {
            gvt_flag_ = false;
        }
    }

    straggler_flags_lock_.unlock();
}

void TimeWarpEventSet::gvtCalcRequest () {
    straggler_flags_lock_.lock();

    std::memcpy(gvt_straggler_flags_.get(), continuous_straggler_flags_.get(),
        num_of_objects_*sizeof(unsigned int));
    gvt_straggler_cnt_ = continuous_straggler_cnt_;
    gvt_flag_ = true;

    straggler_flags_lock_.unlock();
}

bool TimeWarpEventSet::isRollbackPending () {
    return gvt_flag_;
}

} // namespace warped

