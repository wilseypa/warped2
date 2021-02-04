/* 
----notes----
Splitting three main tasks into seperate threads, this class will hold the stuff each task requires

*gvtCntrl will handle estimation and termination

Class houseKeeping() {
    gvt.barrier() <- # of worker threads + 2 (Coordinate the worker threads w/ gCollect and receiveEvent) 
    update removed this --> gvtCntrl.barrier() <- 2 (Sync gvtCntrl w/ fCollect) ==> Figure 3

    gvt.gvtEstCycle <- false
    gvt.gvt <- 0

    gvtContrib <- 0
    fcGvt <- 0
    gvtCycleInterval <- delay (delay > 0: Time between gvt estimation cycles)
    update removed this (moved into gvtCntrl) --> gvt.nextGvt <- 0

    gvtCntrl **thread** (GVT Estimation Task)
    receiveEvent **thread**
    fCollect **thread**
}

***Initially Modeling off TimeWarpGVTManager.hpp***

*/

#ifndef HOUSE_KEEPING_HPP
#define HOUSE_KEEPING_HPP

//which of these are needed, I believe all?
#include <memory>
#include <mutex>
#include <shared_mutex>

// -- Are these needed here or into the threads? --
//#include "TimeWarpEventDispatcher.hpp"
//#include "TimeWarpKernelMessage.hpp"

namespace warped {
    //enum class GVTState { IDLE, LOCAL, GLOBAL };
    //enum class Color { WHITE, RED };

    class HouseKeeping {
        public:
            HouseKeeping();

            virtual void initialize();
            virtual ~HouseKeeping() = default;

            // in the doc these are gvt.value
            // are these values being held in a class somewhere else?
            virtual bool gvtEstCycle() = 0;

            virtual unsigned int gvtCycleInterval() = 0; // <- delay goes here

            virtual int getGVT() { return gVT_; }

        protected:
            unsigned int gVT_ = 0;

            unsigned int gvt_period_;

            unsigned int num_worker_threads_;

    };

}

#endif
