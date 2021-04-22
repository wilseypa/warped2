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
        // find saved state in stateQ with the first timestamp before the timestamp of the straggler
        // coast forward
        // send anti-messages using the records in the outQ and preserve the MPI_Isend pointer to the last message sent in outMessage
        // return
    }

    void Worker::processEvent(int event)
    {
        // Lock e.lp.inQ
        // e <- e.lp.inQ.head()
        // UnLock e.lp.inQ
        // if e.type == ANTIMESSAGE then
            // Find the positive message q that is the target of e
            // if q is in the processed queue then
                // rollback(q->prev)
            // end if
            // Remove q, e and refresh the LTSF queue for that LP
            // return NULL, NULL
        // end if
        // if e.timestamp <= lvt then
            // rollback(e)
        // end if
        // outEventList <- lp.eventExec(e)
        // update LTSF entry for e.LP
        // return e, outEventList
    }

    void Worker::thread() 
    {
        // thread loop
            // if !(e<-ltsf.head()) || gvt.gvtEstCycle then
                // if worker.outMessage != NULL then
                    // worker.outMessage.MPI_Wait()
                // end if
                // gvt.barrier()
                // gvt.barrier()
                // e<-ltsf.refreshEvents(e)
                // if e != NULL then
                    // worker.gvtContrib<-e.timeStamp
                // else
                    // worker.gvtContrib<-+inf
                // end if
                // gvt.barrier()
            // end if
            // if e != NULL then
                // for i <- 1, K0 do
                    // for j <- 1, K1 do
                        // outEventList <- processEvent(e)
                        // for outE in outEventList do
                            // if outE.dest.LP is in LocalLPs then
                                // outE.dest.Lp.inQ.insert(outE)
                            // else
                                // worker.outMessage = MPI_Isend(outE.dest.Lp)
                            // end if
                        // end for
                        // if !(e<-ltsf.head()) then
                            // break
                        // end if
                    // end for
                    // if !(e<- ltsf.refreshEvents(e)) then
                        // break
                    // end if
                // end for
            // end if
        // end thread loop
    }
}