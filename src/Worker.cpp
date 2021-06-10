/*
----notes----


Class Worker() {
    worker.gvtContrib <- 0
    worker.outMessage <- NULL

    function rollback(*e) (rollback to event e)
    function processEvent(*e) (process event from LTSF queue -> check LP -> antimessage -> rollback -> exec)

    worker **thread**
}
*/

#include "Worker.hpp"

namespace warped {
    Worker::Worker() {}

    void Worker::initialize() 
    {
        
    }

    void Worker::rollback(int event)
    {
        timestamp_straggler = event.timestamp
        // find saved state in stateQ with the first timestamp before the timestamp of the straggler
        foreach state in stateQ () {
            if (state.timestamp < timestamp_straggler) {
                coastToPoint = timestamp
            }                
        }
        // coast forward
        // send anti-messages using the records in the outQ and preserve the MPI_Isend pointer to the last message sent in outMessage
        return;
    }

    void Worker::processEvent(int event) // event here is a type, not an int
    {
        // Lock e.lp.inQ
        // e <- e.lp.inQ.head()
        // UnLock e.lp.inQ
        if (e.type == ANTIMESSAGE) {
            // Find the positive message q that is the target of e
            if (q is in the processed queue) {
                rollback(q->prev);
            }
            // Remove q, e and refresh the LTSF queue for that LP
            return (NULL, NULL);
        }
        if (e.timestamp <= lvt) {
            rollback(e);
        }
        outEventList <- lp.eventExec(e) // should put a console log here to mark when an event is actually executed 
        // update LTSF entry for e.LP
        return (e, outEventList);
    }

    void Worker::thread() 
    {
        while(!termination_manager_->terminationStatus()) {
            if (!(e<-ltsf.head()) || gvt.gvtEstCycle) {
                if (worker.outMessage != NULL) {
                    worker.outMessage.MPI_Wait()
                }
                gvt.barrier()
                gvt.barrier()
                e<-ltsf.refreshEvents(e)
                if (e != NULL) {
                    worker.gvtContrib<-e.timeStamp
                } else {
                    worker.gvtContrib<-+inf // fix 
                }
                gvt.barrier()
            }
            if (e != NULL) {
                for i <- 1, K0 {
                    for j <- 1, K1 {
                        outEventList <- processEvent(e)
                        for (outE in outEventList) {
                            if (outE.dest.LP is in LocalLPs) {
                                outE.dest.Lp.inQ.insert(outE)
                            }
                            else {
                                worker.outMessage = MPI_Isend(outE.dest.Lp)
                            }
                        }
                        if (!(e<-ltsf.head())) {
                            break;
                        }
                    }
                    if (!(e<- ltsf.refreshEvents(e))) {
                        break;
                    }
                }
            }
        }
    }
}