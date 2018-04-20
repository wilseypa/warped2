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

    /* Create the input, schedule and processed queues.
     * Create the scheduled event pointer.
     */
    schedule_queue_ = make_unique<bag []>(num_of_bags_);
    unsigned int lp_id = 0;
    for (unsigned int bag_id = 0; bag_id < lps.size(); bag_id++) {
        for (unsigned int i = 0; i < lps[bag_id].size(); i++) {
            input_queue_.push_back(
                    make_unique<std::multiset<std::shared_ptr<Event>, compareEvents>>());
            processed_queue_.push_back(make_unique<std::deque<std::shared_ptr<Event>>>());
            scheduled_event_pointer_.push_back(nullptr);
            schedule_queue_[bag_id].lp_ids_.push_back(lp_id++);
        }
    }

    /* Create the data structure for holding the lowest event timestamp */
    for (unsigned int i = 0; i < num_of_worker_threads; i++) {
        schedule_cycle_.push_back(std::make_tuple(0, (unsigned int)-1, 0));
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
 */
void TimeWarpEventSet::insertEvent (unsigned int lp_id, std::shared_ptr<Event> event) {

    input_queue_[lp_id]->insert(event);
}

unsigned int TimeWarpEventSet::getBag (unsigned int thread_id) {

    /* Calculate the bag to be fetched */
    unsigned int limit = (unsigned int)-1;
    limit -= limit % num_of_bags_;
    unsigned int fetch_index = 0, next_fetch_index = 0;
    do {
        fetch_index = fetch_bag_index_.load();
        next_fetch_index = (fetch_index+1) % limit;
    } while (!std::atomic_compare_exchange_weak(
                        &fetch_bag_index_, &fetch_index, next_fetch_index   ));

    /* Reserve the bag */
    auto bag_id = fetch_index % num_of_bags_;
    while (schedule_queue_[bag_id].is_locked_.test_and_set());

    /* A schedule cycle refers to all bags getting processed once. In a
     * schedule cycle, this function updates the lowest event timestamp
     * processed by a thread.
     */
    auto prev_fetch_index = std::get<0>(schedule_cycle_[thread_id]);

    /* If it is a different schedule cycle.
       NOTE: Atomic variable fetch_bag_index_ can have any value >= 0 and less
             than largest multiple of bag count within capacity of unsigned int.
             Once it reaches the upper limit, fetch_bag_index_ value cycles back
             to 0. This situation may arise when simulation runs for a long time.
             Ideally, fetch_bag_index_ should be designed to have value >= 0 and
             less than bag count. But that would not allow us to determine the
             schedule cycle. So, as a design compromise, fetch_bag_index_ values
             have been designed in the a way mentioned above.
     */
    if ( fetch_index / num_of_bags_ != prev_fetch_index / num_of_bags_ ) {

        std::get<0>(schedule_cycle_[thread_id]) = fetch_index;
        std::get<2>(schedule_cycle_[thread_id]) =
                            std::get<1>(schedule_cycle_[thread_id]);
        std::get<1>(schedule_cycle_[thread_id]) = (unsigned int)-1;
    }

    return bag_id;
}

std::vector<unsigned int> TimeWarpEventSet::fetchLPList (unsigned int bag_id) {

    return schedule_queue_[bag_id].lp_ids_;
}

/*
 *  NOTE: caller must always have the input queue lock for the lp with id lp_id
 */
std::shared_ptr<Event> TimeWarpEventSet::getEvent (unsigned int lp_id, unsigned int thread_id) {

    /*  NOTE: Event also remains in input queue until processing done. If this a
     *  negative event then, a rollback will bring the processed positive event
     *  back to input queue and they will be cancelled.
     */
    if (!input_queue_[lp_id]->empty()) {
        scheduled_event_pointer_[lp_id] = *input_queue_[lp_id]->begin();
        std::get<1>(schedule_cycle_[thread_id]) =
                std::min(   std::get<1>(schedule_cycle_[thread_id]),
                            scheduled_event_pointer_[lp_id]->timestamp() );
    } else {
        scheduled_event_pointer_[lp_id] = nullptr;
    }

    return scheduled_event_pointer_[lp_id];
}

unsigned int TimeWarpEventSet::lowestTimestamp (unsigned int thread_id) {

    return std::min(    std::get<2>(schedule_cycle_[thread_id]),
                        std::get<1>(schedule_cycle_[thread_id])     );
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

    while (event_riterator != processed_queue_[lp_id]->rend() &&
                            (**event_riterator >= *straggler_event)){

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
 *  NOTE: This can only be called by the thread that handles this event for the lp
 *
 *  NOTE: caller must always have the input queue lock for the lp
 *
 *  NOTE: the scheduled_event_pointers are also protected by input queue locks
 */
void TimeWarpEventSet::moveToProcessedQueue (unsigned int lp_id, unsigned int thread_id) {

    assert(scheduled_event_pointer_[lp_id]);

    /* Move the just processed event to the processed queue */
    auto num_erased = input_queue_[lp_id]->erase(scheduled_event_pointer_[lp_id]);
    assert(num_erased == 1);
    unused(num_erased);

    processed_queue_[lp_id]->push_back(scheduled_event_pointer_[lp_id]);

    /* Track the min timestamp for stragglers */
    if (!input_queue_[lp_id]->empty()) {
        auto e = *input_queue_[lp_id]->begin();
        std::get<1>(schedule_cycle_[thread_id]) =
                std::min( std::get<1>(schedule_cycle_[thread_id]), e->timestamp() );
    }
}

void TimeWarpEventSet::makeBagAvailable (unsigned int bag_id) {

    /* Release the bag */
    schedule_queue_[bag_id].is_locked_.clear();
}

/*
 *  NOTE: caller must have the input queue lock for the lp with id lp_id
 */
bool TimeWarpEventSet::cancelEvent (    unsigned int lp_id,
                                        std::shared_ptr<Event> cancel_event,
                                        unsigned int thread_id  ) {

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

    /* Track the min timestamp for stragglers */
    if (!input_queue_[lp_id]->empty()) {
        auto e = *input_queue_[lp_id]->begin();
        std::get<1>(schedule_cycle_[thread_id]) =
                std::min( std::get<1>(schedule_cycle_[thread_id]), e->timestamp() );
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

