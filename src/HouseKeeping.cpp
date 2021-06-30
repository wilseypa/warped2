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
#include "HouseKeeping.hpp"

namespace warped 
{
        HouseKeeping::HouseKeeping(unsigned int num_worker_threads :
                num_worker_threads_(num_worker_threads)) {}
        
        void HouseKeeping::initialize(unsigned int num_local_lps)
            {
                //state_queue_ = make_unique<std::deque<SavedState>[]>(num_local_lps);
                num_local_lps_ = num_local_lps;
                gvt_cntrl_ = make_unique<GvtCntrl>(comm_manager_,event_set_,gvt_manager_,state_manager_,output_manager_,twfs_manager_,termination_manager_,event_dispatcher_);
                // the barrier parameter will be set when gvt_manager is constructed.
                gvt_manager_->setGVTEstCycle(false);
                gvt_manager_->setGVT(0);
                // Thread call is needed to start gvtCntrl after constructor.
                // fCollect Thread call
                fossil_collect_->thread();
                // Receive event Thread call
                receive_event_->thread();
                // is worker called here? multiple workers are created for each individual lp
            }
            
        HouseKeeping::barrier(num_worker_threads_ + 1) {}
    
        
}
