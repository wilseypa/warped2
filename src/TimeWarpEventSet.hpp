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

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize (unsigned int num_of_objects, 
                     unsigned int num_of_schedulers,
                     bool is_lp_migration_on,
                     unsigned int num_of_worker_threads);

    void acquireInputQueueLock (unsigned int obj_id);

    void releaseInputQueueLock (unsigned int obj_id);

    void insertEvent (unsigned int obj_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

    std::shared_ptr<Event> lastProcessedEvent (unsigned int obj_id);

    void rollback (unsigned int obj_id, std::shared_ptr<Event> straggler_event);

    std::unique_ptr<std::vector<std::shared_ptr<Event>>> getEventsForCoastForward (
                        unsigned int obj_id, 
                        std::shared_ptr<Event> straggler_event, 
                        std::shared_ptr<Event> restored_state_event);

    void startScheduling (unsigned int obj_id);

    void replenishScheduler (unsigned int obj_id);

    void cancelEvent (unsigned int obj_id, std::shared_ptr<Event> cancel_event);

    void printEvent (std::shared_ptr<Event> event);

    void fossilCollect (unsigned int fossil_collect_time, unsigned int obj_id);

private:
    // Number of simulation objects
    unsigned int num_of_objects_ = 0;

    // Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> input_queue_lock_;

    // Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> input_queue_;

    // Queues to hold the processed events for each simulation object
    std::vector<std::unique_ptr<std::vector<std::shared_ptr<Event>>>> 
                                                            processed_queue_;

    // Number of event schedulers
    unsigned int num_of_schedulers_ = 0;

    // Lock to protect the schedule queues
    std::unique_ptr<std::mutex []> schedule_queue_lock_;

    // Queues to hold the scheduled events
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> schedule_queue_;

    // Map unprocessed queue to a schedule queue
    std::vector<unsigned int> input_queue_scheduler_map_;

    // LP Migration flag
    bool is_lp_migration_on_;

    // Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_;

    // Event scheduled from all objects
    std::vector<std::shared_ptr<Event>> scheduled_event_pointer_;
};

} // warped namespace

#endif
