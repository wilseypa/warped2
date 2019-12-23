#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <deque>
#include <set>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>

#include "config.h"
#include "LogicalProcess.hpp"
#include "Event.hpp"
#include "utility/memory.hpp"
#include "TicketLock.hpp"
#include "LadderQueue.hpp"
#include "SplayTree.hpp"
#include "CircularQueue.hpp"

namespace warped {

enum class InsertStatus {
    LpOnly,
    StarvedObject,
    SchedEventSwapSuccess,
    SchedEventSwapFailure
};

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize (const std::vector<std::vector<LogicalProcess*>>& lps,
                     unsigned int num_of_lps,
                     bool is_lp_migration_on,
                     unsigned int num_of_worker_threads);

    void acquireInputQueueLock (unsigned int lp_id);

    void releaseInputQueueLock (unsigned int lp_id);

    InsertStatus insertEvent (unsigned int lp_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

#ifdef PARTIALLY_SORTED_LADDER_QUEUE
    unsigned int lowestTimestamp (unsigned int thread_id);
#endif

    std::shared_ptr<Event> lastProcessedEvent (unsigned int lp_id);

    void rollback (unsigned int lp_id, std::shared_ptr<Event> straggler_event);

    std::unique_ptr<std::vector<std::shared_ptr<Event>>> getEventsForCoastForward (
                        unsigned int lp_id, 
                        std::shared_ptr<Event> straggler_event, 
                        std::shared_ptr<Event> restored_state_event);

    void startScheduling (unsigned int lp_id);

    void replenishScheduler (unsigned int lp_id);

    bool cancelEvent (unsigned int lp_id, std::shared_ptr<Event> cancel_event);

    void printEvent (std::shared_ptr<Event> event);

    unsigned int fossilCollect (unsigned int fossil_collect_time, unsigned int lp_id);

    void refreshSceduleQueue(unsigned int thread_id);

    void setWorkerThreadInputQueueMap(std::vector<std::vector<unsigned int>> map);

private:
    // Number of lps
    unsigned int num_of_lps_ = 0;

    // Lock to protect the unprocessed queues
    std::unique_ptr<std::shared_mutex []> input_queue_lock_;

    // Queues to hold the unprocessed events for each lp
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> input_queue_;

    // Queues to hold the processed events for each lp
    std::vector<std::unique_ptr<std::deque<std::shared_ptr<Event>>>> 
                                                            processed_queue_;

    // Number of event schedulers
    unsigned int num_of_schedulers_ = 0;

    // Lock to protect the schedule queues
#ifdef SCHEDULE_QUEUE_SPINLOCKS
    std::unique_ptr<TicketLock []> schedule_queue_lock_;
#else
    std::unique_ptr<std::shared_mutex []> schedule_queue_lock_;
#endif

    // Queues to hold the scheduled events
#if defined(SORTED_LADDER_QUEUE) || defined(PARTIALLY_SORTED_LADDER_QUEUE)
    std::vector<std::unique_ptr<LadderQueue>> schedule_queue_;
#elif defined(SPLAY_TREE)
    std::vector<std::unique_ptr<SplayTree>> schedule_queue_;
#elif defined(CIRCULAR_QUEUE)
    std::vector<std::unique_ptr<CircularQueue>> schedule_queue_;
#else
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> schedule_queue_;
#endif

    // Map unprocessed queue to a schedule queue
    std::vector<unsigned int> input_queue_scheduler_map_; // Get scheduler id from lp_id

    std::vector<std::vector<unsigned int>> worker_thread_input_queue_map_;

    // LP Migration flag
    bool is_lp_migration_on_;

    // Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_; // Get scheduler id from from thread_id

    // Event scheduled from all lps
    std::vector<std::shared_ptr<Event>> scheduled_event_pointer_;
};

} // warped namespace

#endif
