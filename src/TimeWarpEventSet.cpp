#include "TimeWarpEventSet.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

void TimeWarpEventSet::initialize( unsigned int num_of_objects, unsigned int num_of_schedulers ) {

    num_of_objects_ = num_of_objects;
    num_of_schedulers_ = num_of_schedulers;

    //Create the unprocessed and processed queues
    for (unsigned int obj_index = 0; obj_index < num_of_objects; obj_index++) {
        unprocessed_queue_.push_back(new STLLTSFQueue());
        processed_queue_.push_back(new STLLTSFQueue());
    }

    //Create the event schedulers
    for (unsigned int scheduler_index = 0; scheduler_index < num_of_schedulers; scheduler_index++) {
        event_scheduler_.push_back(new TimeWarpEventScheduler());
    }
}

void TimeWarpEventSet::insertEvent( unsigned int obj_id, const std::unique_ptr<Event> event ) {

    unprocessed_queue_lock_.lock();
    unprocessed_queue[obj_id]->push(event);

    //If the event just inserted is at the beginning, update the schedule queue
    Event *peek_event = unprocessed_queue[obj_id]->peek();
    if( event == *peek_event ) {
        TimeWarpEventScheduler *scheduler = event_scheduler_[scheduler_to_obj_map[obj_id]];
        scheduler->acquireScheduleQueueLock();
        if( !scheduler->isObjectScheduled(obj_id) ) {
            scheduler->removeScheduledEvent(obj_id);
            scheduler->insertEvent(obj_id, event);
        }
        scheduler->releaseScheduleQueueLock();
    }
    unprocessed_queue_lock_.unlock();
}

bool TimeWarpEventSet::handleAntiMessage ( 
                                    unsigned int obj_id, 
                                    const std::unique_ptr<Event> cancel_event ) { 

    bool was_event_removed = false;
    unprocessed_queue_lock_.lock();

    for ()
            && !eventWasRemoved) {
        if ((*(multisetIterator[threadId]))->getSender()
                == negativeEvent->getSender()
                && ((*(multisetIterator[threadId]))->getEventId()
                    == negativeEvent->getEventId())) {
            const Event* eventToRemove = *multisetIterator[threadId];
            if (dynamic_cast<const StragglerEvent*>(*(multisetIterator[threadId]))) {
                debug::debugout
                        << "Negative Message Found in Handling Anti-Message .."
                        << endl;
                multisetIterator[threadId]++;
                continue;
            }
            unProcessedQueue[objId]->erase(multisetIterator[threadId]);
            was_event_removed = true;
        } else {
            multisetIterator[threadId]++;
        }
    }
    unprocessed_queue_lock_.unlock();

    return was_event_removed;
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

