// Note this file pair will house the gvtcntrl thread.
#include "HouseKeeping.hpp"

namespace warped
{
    
    class GvtCntrl
    {
        public:

            GvtCntrl::GvtCntrl(
            std::shared_ptr<TimeWarpMPICommunicationManager> comm_manager,
            std::unique_ptr<TimeWarpEventSet> event_set,
            std::unique_ptr<TimeWarpGVTManager> gvt_manager,
            std::unique_ptr<TimeWarpStateManager> state_manager,
            std::unique_ptr<TimeWarpOutputManager> output_manager,
            std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
            std::unique_ptr<TimeWarpTerminationManager> termination_manager,
            std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher);

            
            void GvtCntrl::thread();

        #ifdef TIMEWARP_EVENT_LOG
            // Event log for each worker thread
            std::vector<std::unique_ptr<CircularList<std::string>>> event_log_;
        #endif
        private:

            const std::shared_ptr<TimeWarpMPICommunicationManager> comm_manager_;
            const std::unique_ptr<TimeWarpEventSet> event_set_;
            const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
            const std::unique_ptr<TimeWarpStateManager> state_manager_;
            const std::unique_ptr<TimeWarpOutputManager> output_manager_;
            const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
            const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
            const std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher_;
            unsigned int gvt;
            unsigned int temp_local_min;
            unsigned int prev_gvt;
            unsigned int next_gvt;


    };
}