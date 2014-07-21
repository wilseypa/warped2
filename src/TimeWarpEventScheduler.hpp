#ifndef TIME_WARP_EVENT_SCHEDULER_HPP
#define TIME_WARP_EVENT_SCHEDULER_HPP

/* This class provides the set of APIs needed to handle events in
   the scheduler.
 */

namespace warped {

class LTSFQueue;

class TimeWarpEventScheduler {
public:
    TimeWarpEventScheduler() = default;

    void acquireScheduleQueueLock();

    void releaseScheduleQueueLock();

private:
    //Lock to protect the schedule queue
    std::mutex schedule_queue_lock_;

    //Queue to hold the scheduled events (one event from each object)
    std::unique_ptr<LTSFQueue> schedule_queue_;

};

} // warped namespace

#endif
