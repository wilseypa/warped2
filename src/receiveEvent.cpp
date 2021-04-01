/*

one instance for each cluster node.
takes in all event msgs on the MPI EVENT comm

receiveEvent thread:

while true do
    m <- MPI_Recv(EVENT, MPI_ANY_SOURCE) // receive all events, set them to m

    if m.tag = "Verify_Idle" then
        MPI_Barrier(MSGs_Proc)
    else
        lp <- findLpPtr(m.lp) // find the ptr for the given lp
        lp.inQ.insert(m.e) //use that ptr to insert the event into the queue
    end if
end while

end thread loop

*/
#include "receiveEvent.hpp"

namespace warped {
    receiveEvent::receiveEvent() {}

    void receiveEvent::initialize() 
    {
        WARPED_REGISTER_MSG_HANDLER(receiveEvent, thread, EventMessage);
    }

    void receiveEvent::thread(std::unique_ptr<TimeWarpKernelMessage> kmsg) 
    {

        auto msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(kmsg));
        assert(msg->event != nullptr);

        tw_stats_->upCount(TOTAL_EVENTS_RECEIVED, thread_id);

        termination_manager_->updateMsgCount(-1);

        if(msg->tag_ == "Verify_Idle")
        {
            comm_manager_->waitForMessageProcesses();
        }
        else
        {
            // Will try to figure out if this is needed.
            gvt_manager_->receiveEventUpdate(msg->event, msg->color_);

            // this houses lines 7 and *
            event_dispatcher_->sendLocalEvent(msg->event);
        }
    

    }
}