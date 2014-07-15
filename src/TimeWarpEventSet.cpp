#include "TimeWarpEventSet.hpp"

namespace warped {

TimeWarpEventSet::TimeWarpEventSet() {

}

void TimeWarpEventSet::initialize( unsigned int num_of_objects ) {

}

void TimeWarpEventSet::insertEvent( unsigned int obj_id, const std::unique_ptr<Event> event ) {

    unprocessed_queue_lock_.lock();
    unprocessed_queue[obj_id]->push(event);

    //If the event just inserted is at the beginning, update the schedule queue
    Event *peek_event = unprocessed_queue[obj_id]->peek();
    if( event == *peek_event ) {
        /*LTSF[LTSFByObj[objId]]->getScheduleQueueLock(threadId);
        if (!this->isObjectScheduled(objId)) {
            LTSF[LTSFByObj[objId]]->eraseSkipFirst(objId);
            LTSF[LTSFByObj[objId]]->insertEvent(objId, receivedEvent);
        }
        LTSF[LTSFByObj[objId]]->releaseScheduleQueueLock(threadId);*/
    }
    unprocessed_queue_lock_.unlock();
}

bool TimeWarpEventSet::handleAntiMessage ( 
                                    SimulationObject* object, 
                                    const std::unique_ptr<Event> cancel_event, 
                                    unsigned int thread_id ) {

}

const std::unique_ptr<Event> TimeWarpEventSet::getEvent ( 
                                    SimulationObject* object, 
                                    unsigned int timestamp, 
                                    unsigned int thread_id ) {

    //might need to revert back to old saved file. work under progress.
    const Event* ret = NULL;
    unsigned int objId = simObj->getObjectID()->getSimulationObjectID();
    if (!this->unprocessedQueueLockState[objId]->hasLock(threadId, syncMechanism))
    { this->getunProcessedLock(threadId, objId); }
    if (getQueueEventCount(objId) > 0) {
        //Remove from Unprocessed Queue
        ret = *(unProcessedQueue[objId]->begin());
        //Return NULL if ret is a Straggler/Negative
        if (dynamic_cast<const StragglerEvent*>(ret) || ret->getReceiveTime()
                < simObj->getSimulationTime()) {
            this->releaseunProcessedLock(threadId, objId);
            return NULL;
        }
        unProcessedQueue[objId]->erase(unProcessedQueue[objId]->begin());
        this->releaseunProcessedLock(threadId, objId);
        //Insert into Processed Queue
        if (dynamic_cast<const StragglerEvent*>(ret))
        { ASSERT(false); }
        this->getProcessedLock(threadId, objId);
        processedQueue[objId]->push_back(ret);
        this->releaseProcessedLock(threadId, objId);
    } else {
        this->releaseunProcessedLock(threadId, objId);
    }
    //ASSERT(this->isObjectScheduledBy(threadId, objId));
    return ret;
}

const std::<Event> TimeWarpEventSet::peekEvent ( 
                                    SimulationObject* object, 
                                    unsigned int timestamp, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::fossilCollect ( 
                                    SimulationObject* object, 
                                    unsigned int timestamp, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::fossilCollect ( 
                                    const std::unique_ptr<Event> event, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::rollback (   SimulationObject* object, 
                                    unsigned int rollback_time, 
                                    unsigned int thread_id ) {

}

} // namespace warped

