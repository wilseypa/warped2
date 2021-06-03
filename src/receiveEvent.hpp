#include "HouseKeeping.hpp"
#include "TimeWarpEventDispatcher.hpp"

namespace warped
{
    
    class receiveEvent
    {
        public:

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
            std::unique_ptr<TimeWarpStatistics> tw_stats);

            void receiveEvent::initialize();

            void receiveEvent::thread();
            
        private:

            const std::shared_ptr<TimeWarpMPICommunicationManager> comm_manager_;
            const std::unique_ptr<TimeWarpEventSet> event_set_;
            const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
            const std::unique_ptr<TimeWarpStateManager> state_manager_;
            const std::unique_ptr<TimeWarpOutputManager> output_manager_;
            const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
            const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
            const std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher_;

    };
}