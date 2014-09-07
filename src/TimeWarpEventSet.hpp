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

#include "Event.hpp"
#include "STLLTSFQueue.hpp"
#include "utility/memory.hpp"

namespace warped {

/* Compares two events to see if one has a receive time less than to the other
 */
class compareEvents :
    public std::binary_function<std::shared_ptr<Event>, 
                                    std::shared_ptr<Event>, bool> {
public:
    bool operator() (std::shared_ptr<Event> first, 
                        std::shared_ptr<Event> second) {

        return (first->timestamp() < second->timestamp()
                || first->sender_name_ < second->sender_name_
                || first->rollback_cnt_ < second->rollback_cnt_
                || first->event_type_ < second->event_type_);
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

    std::shared_ptr<Event> readLowestEventFromObj (unsigned int obj_id);

    void replenishScheduler (unsigned int obj_id, 
                                std::shared_ptr<Event> old_event);

    void fossilCollectAll (unsigned int fossil_collect_time);

    void rollback (unsigned int obj_id, unsigned int rollback_time); 

    void handleAntiMessage (unsigned int obj_id, 
                            std::shared_ptr<Event> cancel_event );

private:
    //Number of simulation objects
    unsigned int num_of_objects_;

    //Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> unprocessed_queue_lock_;

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                        compareEvents>>> unprocessed_queue_;

    //Lock to protect the processed queues
    std::unique_ptr<std::mutex []> processed_queue_lock_;

    //Queues to hold the processed events for each simulation object
    std::vector<std::unique_ptr<std::vector<std::shared_ptr<Event>>>> 
                                                        processed_queue_;

    //Number of event schedulers
    unsigned int num_of_schedulers_;

    //Lock to protect the schedule queues
    std::unique_ptr<std::mutex []> schedule_queue_lock_;

    //Queues to hold the scheduled events
    std::vector<std::unique_ptr<LTSFQueue>> schedule_queue_;

    //Map unprocessed queue to a schedule queue
    std::vector<unsigned int> unprocessed_queue_scheduler_map_;

    //Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_;
};

} // warped namespace

#endif
