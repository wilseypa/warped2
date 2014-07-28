#include "TimeWarpEventScheduler.hpp"

namespace warped {

void TimeWarpEventScheduler::acquireScheduleQueueLock() {

    schedule_queue_lock_.lock();
}

void TimeWarpEventScheduler::releaseScheduleQueueLock(int threadId) {

    schedule_queue_lock_.unlock();
}

unsigned int TimeWarpEventScheduler::nextScheduledEvent() {

    const std::unique_ptr<Event> event = nullptr;
    schedule_queue_lock_.lock();
    if (schedule_queue_.size()) {
        event = *(schedule_queue_.begin());
    }
    schedule_queue_lock_.unlock();
    return event;
}

bool TimeWarpEventScheduler::isScheduleQueueEmpty() {

    schedule_queue_lock_.lock();
    bool empty_status = schedule_queue_.empty();
    schedule_queue_lock_.unlock();
    return empty_status;
}

void TimeWarpEventScheduler::setLowestObjectPosition( unsigned int obj_id ) {
    this->getScheduleQueueLock(threadId);
    lowestObjectPosition->at(index) = NULL;
    this->releaseScheduleQueueLock(threadId);
}

void TimeWarpEventScheduler::insertEvent( unsigned int obj_id, 
                                          const std::unique_ptr<Event> new_event ) {

    lowestObjectPosition->at(objId) = *(scheduleQueue->insert(newEvent));
}

void TimeWarpEventScheduler::eraseSkipFirst( unsigned int obj_id ) {

    if (lowestObjectPosition->at(objId) != NULL) {
        scheduleQueue->erase(lowestObjectPosition->at(objId));
    }
}

const Event* TimeWarpEventScheduler::peek(int threadId) {

    const Event* ret = NULL;
    this->getScheduleQueueLock(threadId);
    if (scheduleQueue->size() > 0) {
        ret = *(scheduleQueue->begin());
        unsigned int objId = ret->getReceiver().getSimulationObjectID();
        getObjectLock(threadId, objId);
        scheduleQueue->erase(scheduleQueue->begin());
        lowestObjectPosition->at(objId) = NULL;
    }
    this->releaseScheduleQueueLock(threadId);
    return ret;
}

void TimeWarpEventScheduler::acquireObjectLock( unsigned int obj_id ) {

    object_lock_->at(obj_id).lock();
}

void TimeWarpEventScheduler::releaseObjectLock( unsigned int obj_id ) {

    object_lock_->at(obj_id).unlock();
}

} // namespace warped

