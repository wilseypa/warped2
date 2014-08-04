#include "TimeWarpEventScheduler.hpp"

namespace warped {

TimeWarpEventScheduler::TimeWarpEventScheduler() {

    schedule_queue_ = new STLLTSFQueue();
}

void TimeWarpEventScheduler::acquireScheduleQueueLock() {

    schedule_queue_lock_.lock();
}

void TimeWarpEventScheduler::releaseScheduleQueueLock() {

    schedule_queue_lock_.unlock();
}

const std::shared_ptr<Event> TimeWarpEventScheduler::peekLowestScheduledEvent() {

    const std::shared_ptr<Event> event = nullptr;
    schedule_queue_lock_.lock();
    if (schedule_queue_.size()) {
        event = schedule_queue_.peek();
    }
    schedule_queue_lock_.unlock();
    return event;
}

void TimeWarpEventScheduler::scheduleNewEvent( const std::shared_ptr<Event> new_event ) {

    schedule_queue_lock_.lock();
    schedule_queue_.push(new_event);
    schedule_queue_lock_.unlock();
}

const std::shared_ptr<Event> TimeWarpEventScheduler::getEventForExecution() {

    const std::shared_ptr<Event> event = nullptr;
    schedule_queue_lock_.lock();
    if (schedule_queue_.size()) {
        event = schedule_queue_.peek();
        unsigned int obj_id; // needs obj_name to local id map
        acquireObjectLock(obj_id);
        schedule_queue_.pop();
    }
    schedule_queue_lock_.unlock();
    return ret;
}

void TimeWarpEventScheduler::acquireObjectLock( unsigned int obj_id ) {

    object_lock_[obj_id].lock();
}

void TimeWarpEventScheduler::releaseObjectLock( unsigned int obj_id ) {

    object_lock_[obj_id].unlock();
}

} // namespace warped

