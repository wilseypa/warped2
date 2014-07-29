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

    unsigned int TimeWarpEventScheduler::nextScheduledEvent();

    bool TimeWarpEventScheduler::isScheduleQueueEmpty();

    void TimeWarpEventScheduler::acquireObjectLock( unsigned int obj_id );

    void TimeWarpEventScheduler::releaseObjectLock( unsigned int obj_id );

private:
    //Lock to protect the schedule queue
    std::mutex schedule_queue_lock_;

    //Queue to hold the scheduled events (one event from each object)
    std::unique_ptr<LTSFQueue> schedule_queue_;

    //Object locks
    std::vector<std::mutex> object_lock_;
};

} // warped namespace

#endif
