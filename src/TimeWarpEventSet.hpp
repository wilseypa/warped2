#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>

#include "Event.hpp"
#include "utility/memory.hpp"

namespace warped {

/* Compares two events to see if one has a receive time less than to the other
 */
struct compareEvents {
public:
    bool operator() (const std::shared_ptr<Event>& first, 
                        const std::shared_ptr<Event>& second) const {

        bool is_less = false;
        if (first->timestamp() > second->timestamp()) {
            is_less = false;
        } else if (first->timestamp() == second->timestamp()) {
            if (first->sender_name_.compare(second->sender_name_) > 0) {
                is_less = false;
            } else if (first->sender_name_.compare(second->sender_name_) == 0) {
                if (first->send_time_ < second->send_time_) {
                    is_less = true;
                } else if (first->send_time_ == second->send_time_) {
#if ROLLBACK_CNT_ACTIVE
                    if (first->rollback_cnt_ > second->rollback_cnt_) {
                        is_less = false;
                    } else if (first->rollback_cnt_ == second->rollback_cnt_) {
                        is_less = (first->event_type_ < second->event_type_) ? true : false;
                    } else {
                        is_less = true;
                    }
#else
                    is_less = (first->event_type_ < second->event_type_) ? true : false;
#endif
                } else {
                    is_less = false;
                }
            } else {
                is_less = false;
            }
        } else {
            is_less = true;
        }
        return is_less;
    }
};

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize (unsigned int num_of_objects, 
                     unsigned int num_of_schedulers,
                     unsigned int num_of_worker_threads);

    void insertEvent (unsigned int obj_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

    std::unique_ptr<std::vector<std::shared_ptr<Event>>> 
                            getEventsForCoastForward (unsigned int obj_id);

    void startScheduling (unsigned int obj_id);

    void fossilCollectAll (unsigned int fossil_collect_time);

    void rollback (unsigned int obj_id, unsigned int restored_time, 
                                std::shared_ptr<Event> straggler_event);

private:
    // Number of simulation objects
    unsigned int num_of_objects_ = 0;

    // Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> input_queue_lock_;

    // Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> input_queue_;

    // Number of event schedulers
    unsigned int num_of_schedulers_ = 0;

    // Lock to protect the schedule queues
    std::unique_ptr<std::mutex []> schedule_queue_lock_;

    // Queues to hold the scheduled events
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> schedule_queue_;

    // Map unprocessed queue to a schedule queue
    std::vector<unsigned int> input_queue_scheduler_map_;

    // Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_;

    // Position of the event scheduled from an object
    std::vector<std::multiset<std::shared_ptr<Event>,
            compareEvents>::iterator> scheduled_event_pointer_;

    // Position of the lowest event in an object
    std::vector<std::multiset<std::shared_ptr<Event>,
            compareEvents>::iterator> lowest_event_pointer_;

    // Position of the straggler event of an object
    std::vector<std::multiset<std::shared_ptr<Event>,
            compareEvents>::iterator> straggler_event_pointer_;
};

} // warped namespace

#endif
