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
                     unsigned int num_of_worker_threads);

    void acquireInputQueueLock (unsigned int obj_id);

    void releaseInputQueueLock (unsigned int obj_id);

    void insertEvent (unsigned int obj_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

    bool isRollbackRequired(unsigned int obj_id);

    void lastProcessedEvent (unsigned int obj_id, std::shared_ptr<Event> event);

    void processedEvent (unsigned int obj_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getLowestUnprocessedEvent (unsigned int obj_id);

    void resetLowestUnprocessedEvent (unsigned int obj_id);

    std::unique_ptr<std::vector<std::shared_ptr<Event>>> getEventsForCoastForward (
                        unsigned int obj_id, 
                        std::shared_ptr<Event> straggler_event, 
                        std::shared_ptr<Event> restored_state_event);

    void startScheduling (unsigned int obj_id);

    void cancelEvent (unsigned int obj_id, std::shared_ptr<Event> cancel_event);

    void fossilCollectAll (unsigned int fossil_collect_time);

private:
    // Number of simulation objects
    unsigned int num_of_objects_ = 0;

    // Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> input_queue_lock_;

    // Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> input_queue_;

    // Keep track of which events have been processed.
    // This is needed to weed out new events from coast forwarded list.
    std::vector<std::unique_ptr<
        std::unordered_map<std::shared_ptr<Event>, bool>>> track_processed_event_;

    // Last processed event record for all objects
    std::vector<std::shared_ptr<Event>> last_processed_event_pointer_;

    // Lowest unprocessed event record for all objects
    std::vector<std::shared_ptr<Event>> lowest_unprocessed_event_pointer_;

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

    // Event scheduled from all objects
    std::vector<std::shared_ptr<Event>> scheduled_event_pointer_;
};

} // warped namespace

#endif
