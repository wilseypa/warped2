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
#include <memory>
#include <atomic>

#include "LogicalProcess.hpp"
#include "Event.hpp"
#include "utility/memory.hpp"
#include "TicketLock.hpp"
#include "LadderQueue.hpp"

namespace warped {

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize (const std::vector<std::vector<LogicalProcess*>>& lps,
                     unsigned int num_of_lps,
                     bool is_lp_migration_on,
                     unsigned int num_of_worker_threads);

    void acquireInputQueueLock (unsigned int lp_id);

    void releaseInputQueueLock (unsigned int lp_id);

    void insertEvent (unsigned int lp_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

#ifdef LADDER_QUEUE_SCHEDULER
#ifdef PARTIALLY_UNSORTED_EVENT_SET
    unsigned int lowestTimestamp (unsigned int thread_id);
#endif
#endif

    std::shared_ptr<Event> lastProcessedEvent (unsigned int lp_id);

    void rollback (unsigned int lp_id, std::shared_ptr<Event> straggler_event);

    std::unique_ptr<std::vector<std::shared_ptr<Event>>> getEventsForCoastForward (
                        unsigned int lp_id, 
                        std::shared_ptr<Event> straggler_event, 
                        std::shared_ptr<Event> restored_state_event);

    void startScheduling (unsigned int lp_id);

    void replenishScheduler (unsigned int lp_id);

    void cancelEvent (unsigned int lp_id, std::shared_ptr<Event> cancel_event);

    void printEvent (std::shared_ptr<Event> event);

    unsigned int fossilCollect (unsigned int fossil_collect_time, unsigned int lp_id);

private:
    // Number of lps
    unsigned int num_of_lps_ = 0;

    // Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> input_queue_lock_;

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
    std::unique_ptr<std::mutex []> schedule_queue_lock_;
#endif

    // Queues to hold the scheduled events
#ifdef LADDER_QUEUE_SCHEDULER
    std::vector<std::unique_ptr<LadderQueue>> schedule_queue_;
#else
    std::vector<std::unique_ptr<std::multiset<std::shared_ptr<Event>, 
                                            compareEvents>>> schedule_queue_;
#endif

    // Map unprocessed queue to a schedule queue
    std::vector<unsigned int> input_queue_scheduler_map_;

    // LP Migration flag
    bool is_lp_migration_on_;

    // Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_;

    // Event scheduled from all lps
    std::vector<std::shared_ptr<Event>> scheduled_event_pointer_;
};

} // warped namespace

#endif
