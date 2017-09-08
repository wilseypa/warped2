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
#include "CircularList.hpp"

namespace warped {

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize (const std::vector<std::vector<LogicalProcess*>>& lps,
                     unsigned int num_of_lps,
                     unsigned int num_of_worker_threads);

    void acquireInputQueueLock (unsigned int lp_id);

    void releaseInputQueueLock (unsigned int lp_id);

    void insertEvent (unsigned int lp_id, std::shared_ptr<Event> event);

    std::shared_ptr<Event> getEvent (unsigned int thread_id);

    unsigned int lowestTimestamp (unsigned int thread_id);

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

    // Number of bags
    unsigned int num_of_bags_ = 0;

    // Lock to protect the schedule queues
#ifdef SCHEDULE_QUEUE_SPINLOCKS
    std::unique_ptr<TicketLock []> schedule_queue_lock_;
#else
    std::unique_ptr<std::mutex []> schedule_queue_lock_;
#endif

    // Queues to hold the scheduled events
    CircularList<std::unique_ptr<std::shared_ptr<Event> []> > schedule_queue_;

    // Map unprocessed queue to a bag
    std::vector<unsigned int> input_queue_bag_map_;

    // Event scheduled from all lps
    std::vector<std::shared_ptr<Event>> scheduled_event_pointer_;
};

} // warped namespace

#endif
