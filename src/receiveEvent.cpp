#include "receiveEvent.hpp"

namespace warped {
    receiveEvent::receiveEvent(
    unsigned int num_worker_threads,
    bool is_lp_migration_on,
    unsigned int num_refresh_per_gvt,
    unsigned int num_events_per_refresh,
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
    std::unique_ptr<TimeWarpEventSet> event_set,
    std::unique_ptr<TimeWarpGVTManager> gvt_manager,
    std::unique_ptr<TimeWarpStateManager> state_manager,
    std::unique_ptr<TimeWarpOutputManager> output_manager,
    std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
    std::unique_ptr<TimeWarpTerminationManager> termination_manager,
    std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher,
    std::unique_ptr<TimeWarpStatistics> tw_stats) :
        num_worker_threads_(num_worker_threads),
        is_lp_migration_on_(is_lp_migration_on),
        num_refresh_per_gvt_(num_refresh_per_gvt), num_events_per_refresh_(num_events_per_refresh), 
        comm_manager_(comm_manager), event_set_(std::move(event_set)), 
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
        termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}

    void receiveEvent::initialize() 
    {
        WARPED_REGISTER_MSG_HANDLER(receiveEvent, thread, EventMessage);
    }

    void receiveEvent::thread(std::unique_ptr<TimeWarpKernelMessage> kmsg) 
    {
        // Will run until this thread is destroyed
        while(!termination_manager_->terminationStatus()) {
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
}