/* 
----notes----
Splitting three main tasks into seperate threads, this class will hold the stuff each task requires

*gvtCntrl will handle estimation and termination

Class houseKeeping() {
    gvt.barrier() <- # of worker threads + 1 (Coordinate the worker threads w/ gCollect and receiveEvent) 

    gvt.gvtEstCycle <- false
    gvt.gvtCycleInterval <- delay (delay > 0: Time between gvt estimation cycles)
    gvt.gvt <- 0

    gvtCntrl **thread** (GVT Estimation Task)
    receiveEvent **thread**
    fCollect **thread**
}
*/
#include HouseKeeping.hpp

namespace warped 
{
        HouseKeeping::HouseKeeping(unsigned int num_worker_threads :
                num_worker_threads_(num_worker_threads)) {}
        
        void HouseKeeping::initialize()
            {

            }
            
        gvt.barrier(num_worker_threads_ + 1)
        
        HouseKeeping::gvtCntrl() {}
        
        HouseKeeping::receiveEvent() {}
        
        HouseKeeping::fCollect() {}
        
}
