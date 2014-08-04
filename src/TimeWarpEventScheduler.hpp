#ifndef TIME_WARP_EVENT_SCHEDULER_HPP
#define TIME_WARP_EVENT_SCHEDULER_HPP

/* This class provides the set of APIs needed to handle events in
   the scheduler.
 */

namespace warped {

class LTSFQueue;

class TimeWarpEventScheduler {
public:
    TimeWarpEventScheduler();

    void acquireScheduleQueueLock();

    void releaseScheduleQueueLock();

    const std::shared_ptr<Event> peekLowestScheduledEvent();

    void scheduleNewEvent( const std::shared_ptr<Event> new_event );

    const std::shared_ptr<Event> getEventForExecution();

    void acquireObjectLock( unsigned int obj_id );

    void releaseObjectLock( unsigned int obj_id );

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
