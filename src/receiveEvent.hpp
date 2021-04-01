#include "HouseKeeping.hpp"
#include "TimeWarpEventDispatcher.hpp"

namespace warped
{
    
    class receiveEvent
    {
        public:

            receiveEvent::receiveEvent();

            void receiveEvent::thread();
            void receiveEvent::initialize();

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