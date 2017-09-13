#include <algorithm>
#include <cassert>
#include <string>

#include "TimeWarpEventSet.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpEventSet::initialize (const std::vector<std::vector<LogicalProcess*>>& lps,
                                   unsigned int num_of_lps,
                                   unsigned int num_of_worker_threads) {

    num_of_lps_  = num_of_lps;
    num_of_bags_ = lps.size();

    /* Create the input and schedule queue locks */
    input_queue_lock_ = make_unique<std::mutex []>(num_of_lps_);

#ifdef SCHEDULE_QUEUE_SPINLOCKS
    schedule_queue_lock_ = make_unique<TicketLock []>(num_of_bags_);
#else
    schedule_queue_lock_ = make_unique<std::mutex []>(num_of_bags_);
#endif

    /* Create the input, schedule and processed queues.
       Create the input queue-bag map and scheduled event pointer.
     */
    schedule_queue_ = make_unique<bag []>(num_of_bags_);
    for (unsigned int bag_id = 0; bag_id < lps.size(); bag_id++) {
        unsigned int num_lps = lps[bag_id].size();
        schedule_queue_[bag_id].content_ = make_unique<std::shared_ptr<Event> []>(num_lps);

        for (unsigned int lp_id = 0; lp_id < num_lps; lp_id++) {
            input_queue_.push_back(
                    make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
            processed_queue_.push_back(make_unique<std::deque<std::shared_ptr<Event>>>());
            scheduled_event_pointer_.push_back(nullptr);
            input_queue_bag_map_.push_back(bag_id);
        }
    }

    /* Create the data structure for holding the lowest event timestamp */
    for (unsigned int i = 0; i < num_of_worker_threads; i++) {
        lowest_timestamp_thread_map_.push_back(std::make_pair(false, 0));
    }
}

void TimeWarpEventSet::acquireInputQueueLock (unsigned int lp_id) {

    input_queue_lock_[lp_id].lock();
}

void TimeWarpEventSet::releaseInputQueueLock (unsigned int lp_id) {

    input_queue_lock_[lp_id].unlock();
}

/*
 *  NOTE: caller must always have the input queue lock for the lp with id lp_id
 *
 *  NOTE: scheduled_event_pointer is also protected by the input queue lock
 */
void TimeWarpEventSet::insertEvent (    unsigned int lp_id,
                                        std::shared_ptr<Event> event,
                                        unsigned int thread_id      ) {

    // Always insert event into input queue
    input_queue_[lp_id]->insert(event);

    if (scheduled_event_pointer_[lp_id] == nullptr) {
        /* If no event is currently scheduled. This can only happen if the thread that
           handles events for lp with id lp_id has determined that there are no more
           events left in it's input queue.
         */
        assert(input_queue_[lp_id]->size() == 1);
        unsigned int bag_id = input_queue_bag_map_[lp_id];
        schedule_queue_lock_[bag_id].lock();
        schedule_queue_[bag_id].content_[schedule_queue_[bag_id].content_size_++] = event;
        schedule_queue_lock_[bag_id].unlock();
        scheduled_event_pointer_[lp_id] = event;

        /* Update the lowest timestamp for the event inserted */
        updateLowestTimestamp(thread_id, event->timestamp());

    } else {
        auto smallest_event = *input_queue_[lp_id]->begin();
        if (smallest_event != scheduled_event_pointer_[lp_id]) {

            /* If the pointer comparison of the smallest event does not match
               scheduled event, that means we should update the schedule queue.
             */
            unsigned int bag_id = input_queue_bag_map_[lp_id];
            schedule_queue_lock_[bag_id].lock();
            for (unsigned int i = 0; i < schedule_queue_[bag_id].content_size_; i++) {
                if (schedule_queue_[bag_id].content_[i] == scheduled_event_pointer_[lp_id]) {
                    schedule_queue_[bag_id].content_[i] = smallest_event;
                    scheduled_event_pointer_[lp_id] = smallest_event;

                    /* Update the lowest timestamp for smallest event */
                    updateLowestTimestamp(thread_id, smallest_event->timestamp());

                    break;
                }
            }
            schedule_queue_lock_[bag_id].unlock();
        }
    }
}

/*
 *  NOTE: caller must always have the input queue lock for the lp with id lp_id
 */
std::vector<std::shared_ptr<Event>> TimeWarpEventSet::getEvents (unsigned int thread_id) {

    /* Calculate the bag to be fetched */
    unsigned int old_bag_id = 0;
    unsigned int new_bag_id = 0;
    do {
        old_bag_id = fetch_bag_id_;
        new_bag_id = (old_bag_id+1) % num_of_bags_;

    } while ( !fetch_bag_id_.compare_exchange_weak(old_bag_id, new_bag_id) );

    schedule_queue_lock_[new_bag_id].lock();

    std::vector<std::shared_ptr<Event>> events;
    for (unsigned int i = 0; i < schedule_queue_[new_bag_id].content_size_; i++) {
        auto event = schedule_queue_[new_bag_id].content_[i];
        assert(event);
        events.push_back(event);

        /* Update the lowest timestamp for event processed */
        updateLowestTimestamp(thread_id, event->timestamp());
    }
    schedule_queue_[new_bag_id].content_size_ = 0;

    /* NOTE: scheduled_event_pointer is not changed here so that other threads
       will not schedule new events and this thread can move events into processed
       queue and update schedule queue correctly.

       NOTE: Event also remains in input queue until processing done. If this a
       negative event then, a rollback will bring the processed positive event
       back to input queue and they will be cancelled.
     */
    schedule_queue_lock_[new_bag_id].unlock();

    return events;
}

unsigned int TimeWarpEventSet::lowestTimestamp (unsigned int thread_id) {

    lowest_timestamp_thread_map_[thread_id].first = false;
    return lowest_timestamp_thread_map_[thread_id].second;
}

/* NOTE: Private member function */
/* Update the timestamp value in lowest timestamp-thread map */
void TimeWarpEventSet::updateLowestTimestamp (unsigned int thread_id, unsigned int timestamp) {

    auto status = lowest_timestamp_thread_map_[thread_id].first;
    if (!status) {
        lowest_timestamp_thread_map_[thread_id].first  = true;
        lowest_timestamp_thread_map_[thread_id].second = timestamp;

    } else {
        lowest_timestamp_thread_map_[thread_id].second =
                std::min(lowest_timestamp_thread_map_[thread_id].second, timestamp);
    }
}

/*
 *  NOTE: caller must have the input queue lock for the lp with id lp_id
 */
std::shared_ptr<Event> TimeWarpEventSet::lastProcessedEvent (unsigned int lp_id) {

    return ((processed_queue_[lp_id]->size()) ? processed_queue_[lp_id]->back() : nullptr);
}

/*
 *  NOTE: caller must have the input queue lock for the lp with id lp_id
 */
void TimeWarpEventSet::rollback (unsigned int lp_id, std::shared_ptr<Event> straggler_event) {

    /* Every event GREATER OR EQUAL to straggler event must remove from the
       processed queue and reinserted back into input queue. EQUAL will ensure
       that a negative message will properly be cancelled out.
     */
    auto event_riterator = processed_queue_[lp_id]->rbegin();  // Starting with largest event

    while (event_riterator != processed_queue_[lp_id]->rend() && (**event_riterator >= *straggler_event)){

        auto event = std::move(processed_queue_[lp_id]->back()); // Starting from largest event
        assert(event);
        processed_queue_[lp_id]->pop_back();
        input_queue_[lp_id]->insert(event);
        event_riterator = processed_queue_[lp_id]->rbegin();
    }
}

/*
 *  NOTE: caller must have the input queue lock for the lp with id lp_id
 */
std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
    TimeWarpEventSet::getEventsForCoastForward (
                                unsigned int lp_id, 
                                std::shared_ptr<Event> straggler_event, 
                                std::shared_ptr<Event> restored_state_event) {

    /* To avoid error if asserts are disabled */
    unused(straggler_event);

    /* Restored state event is the last event to contribute to the current state of
       the lpt. All events GREATER THAN this event but LESS THAN the straggler event
       must be "coast forwarded" so that the state remains consistent.

       It is assumed that all processed events GREATER THAN OR EQUAL to the straggler
       event have been moved from the processed queue to the input queue with a call
       to rollback().

       All coast forwared events remain in the processed queue.
     */
    auto events = make_unique<std::vector<std::shared_ptr<Event>>>();

    auto event_riterator = processed_queue_[lp_id]->rbegin();  // Starting with largest event

    while ((event_riterator != processed_queue_[lp_id]->rend()) && (**event_riterator > *restored_state_event)) {

        assert(*event_riterator);
        assert(**event_riterator < *straggler_event);
        // Events are in order of LARGEST to SMALLEST
        events->push_back(*event_riterator);
        event_riterator++;
    }

    return (std::move(events));
}

/*
 *  NOTE: call must always have input queue lock for the lp which corresponds to lp_id
 *
 *  NOTE: This is called in the case of an negative message and no event is processed.
 *
 *  NOTE: This can only be called by the thread that handles events for the lp with id lp_id
 *
 */
void TimeWarpEventSet::startScheduling (unsigned int lp_id, unsigned int thread_id) {

    /* Add pointer to next event into the scheduler if input queue is
       not empty for the given lp, otherwise set to nullptr
     */
    if (!input_queue_[lp_id]->empty()) {
        scheduled_event_pointer_[lp_id] = *input_queue_[lp_id]->begin();
        unsigned int bag_id = input_queue_bag_map_[lp_id];
        schedule_queue_lock_[bag_id].lock();
        schedule_queue_[bag_id].content_[schedule_queue_[bag_id].content_size_++] =
                                                            scheduled_event_pointer_[lp_id];
        schedule_queue_lock_[bag_id].unlock();

        /* Update the lowest timestamp for the event inserted */
        updateLowestTimestamp(thread_id, scheduled_event_pointer_[lp_id]->timestamp());

    } else {
        scheduled_event_pointer_[lp_id] = nullptr;
    }
}

/*
 *  NOTE: This can only be called by the thread that handles events for the lps with id lp_ids
 *
 *  NOTE: caller must always have the input queue locks for the lps which correspond to lp_ids
 *
 *  NOTE: the scheduled_event_pointers are also protected by input queue locks
 */
void TimeWarpEventSet::replenishScheduler ( std::vector<unsigned int> lp_ids,
                                            unsigned int thread_id  ) {

    if (!lp_ids.size()) return;

    unsigned int bag_id = input_queue_bag_map_[lp_ids[0]];
    schedule_queue_lock_[bag_id].lock();
    for (auto lp_id : lp_ids) {
        /* Something is completely wrong if there is no scheduled event
           because an event from that lp was just processed.
         */
        assert(scheduled_event_pointer_[lp_id]);

        /* Move the just processed event to the processed queue */
        auto num_erased = input_queue_[lp_id]->erase(scheduled_event_pointer_[lp_id]);
        assert(num_erased == 1);
        unused(num_erased);

        processed_queue_[lp_id]->push_back(scheduled_event_pointer_[lp_id]);

        /* Update scheduler with new event for the lp the previous event was executed for
           NOTE: A pointer to the scheduled event will remain in the input queue
         */
        if (!input_queue_[lp_id]->empty()) {
            scheduled_event_pointer_[lp_id] = *input_queue_[lp_id]->begin();
            schedule_queue_[bag_id].content_[schedule_queue_[bag_id].content_size_++] =
                                                            scheduled_event_pointer_[lp_id];

            /* Update the lowest timestamp for the event inserted */
            updateLowestTimestamp(thread_id, scheduled_event_pointer_[lp_id]->timestamp());

        } else {
            scheduled_event_pointer_[lp_id] = nullptr;
        }
    }
    schedule_queue_lock_[bag_id].unlock();
}

bool TimeWarpEventSet::cancelEvent (unsigned int lp_id, std::shared_ptr<Event> cancel_event) {

    bool found = false;
    auto neg_iterator = input_queue_[lp_id]->find(cancel_event);
    assert(neg_iterator != input_queue_[lp_id]->end());
    auto pos_iterator = std::next(neg_iterator);
    assert(pos_iterator != input_queue_[lp_id]->end());

    if (**pos_iterator == **neg_iterator) {
        input_queue_[lp_id]->erase(neg_iterator);
        input_queue_[lp_id]->erase(pos_iterator);
        found = true;
    }

    return found;
}

// For debugging
void TimeWarpEventSet::printEvent (std::shared_ptr<Event> event) {
    std::cout << "\tSender:     " << event->sender_name_                  << "\n"
              << "\tReceiver:   " << event->receiverName()                << "\n"
              << "\tSend time:  " << event->send_time_                    << "\n"
              << "\tRecv time:  " << event->timestamp()                   << "\n"
              << "\tGeneratrion:" << event->generation_                   << "\n"
              << "\tType:       " << (unsigned int)event->event_type_     << "\n";
}

unsigned int TimeWarpEventSet::fossilCollect (unsigned int fossil_collect_time, unsigned int lp_id) {

    unsigned int count = 0;

    if (processed_queue_[lp_id]->empty()) {
        return count;
    }

    if (fossil_collect_time == (unsigned int)-1) {
        count = processed_queue_[lp_id]->size();
        processed_queue_[lp_id]->clear();
        return count;
    }

    auto event_iterator = processed_queue_[lp_id]->begin();
    while ((event_iterator != std::prev(processed_queue_[lp_id]->end())) &&
           ((*event_iterator)->timestamp() < fossil_collect_time)) {
        processed_queue_[lp_id]->pop_front();
        event_iterator = processed_queue_[lp_id]->begin();
        count++;
    }

    return count;
}

} // namespace warped

