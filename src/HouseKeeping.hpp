/* 
----notes----
Splitting three main tasks into seperate threads, this class will hold the stuff each task requires

*gvtCntrl will handle estimation and termination

condintional instantiation of receiveEvent during parallel sim

Class houseKeeping() {
    gvt.barrier() <- # of worker threads + 1? (Coordinate the worker threads w/ gCollect and receiveEvent) 

    gvt.gvtEstCycle <- false
    gvt.gvt <- 0

    gvtContrib <- 0
    fcGvt <- 0
    gvtCycleInterval <- delay (delay > 0: Time between gvt estimation cycles)

    gvtCntrl **thread** (GVT Estimation Task)
    receiveEvent **thread**
    fCollect **thread**
}

***Initially Modeling off TimeWarpGVTManager.hpp***

*/

#ifndef HOUSE_KEEPING_HPP
#define HOUSE_KEEPING_HPP

#include <memory>
#include <mutex>


#include "fCollect.hpp"
#include "receiveEvent.hpp"
#include "TimeWarpGvtCntrl.hpp"


#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "LogicalProcess.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp" 
#include <shared_mutex>

//#include "TimeWarpEventDispatcher.hpp"
//#include "TimeWarpKernelMessage.hpp"

namespace warped {

    class HouseKeeping {
        public:
            HouseKeeping();

            virtual void initialize(unsigned int num_local_lps);
            virtual ~HouseKeeping() = default;

            virtual void barrier(unsigned int num_worker_threads)) {}

            virtual bool gvtEstCycle() = false;

            virtual unsigned int gvtCycleInterval(unsigned int delay) = 0;

            virtual int getGVT() { return gVT_; }
            
            const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
            const std::unique_ptr<TimeWarpEventSet> event_set_;
            const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
            const std::unique_ptr<TimeWarpStateManager> state_manager_;
            const std::unique_ptr<TimeWarpOutputManager> output_manager_;
            const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
            const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
            const std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher_;
            const std::unique_ptr<GvtCntrl> gvt_cntrl_;

            const std::unique_ptr<receiveEvent> receive_event_;
            const std::unique_ptr<fCollect> fossil_collect_;

        protected:
            unsigned int gVT_ = 0;

            unsigned int gvt_period_;

            unsigned int num_worker_threads_;

            unsigned int num_local_lps_;

            unsigned int delay_;
    };

}

#endif
