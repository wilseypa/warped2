#include "TimeWarpGvtCntrl.hpp" 

namespace warped
{

        GvtCntrl::GvtCntrl(
            std::shared_ptr<TimeWarpMPICommunicationManager> comm_manager,
            std::unique_ptr<TimeWarpEventSet> event_set,
            std::unique_ptr<TimeWarpGVTManager> gvt_manager,
            std::unique_ptr<TimeWarpStateManager> state_manager,
            std::unique_ptr<TimeWarpOutputManager> output_manager,
            std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
            std::unique_ptr<TimeWarpTerminationManager> termination_manager,
            std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher :
                comm_manager_(std::move(comm_manager)), event_set_(std::move(event_set)), 
                gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
                output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
                termination_manager_(std::move(termination_manager))) {}

            void GvtCntrl::thread()
            {                
                gvt = gvt_manager_->getGVT();
                
                // Will run until this thread is destroyed
                while(!termination_manager_->terminationStatus()) {

                    if(comm_manager_->getIDGVT() == 0) {
                        sleep(gvt_manager_->getGVTPeriod());
                    }
                    
                    if(comm_manager_->getNumProcessesGVT() > 1) {
                        comm_manager_->waitForGVT();
                    }
                    
                    gvt_manager_->setGVTEstCycle(true);

                    // This aligns with line 9 of the algorithm loop.
                    gvt_manager_->workerThreadGVTBarrierSync();
                    if(comm_manager_->getNumProcessesGVT() > 1){
                        if(comm_manager_->getID() == 0){
                            // Move to comm manager.
                            comm_manager_->broadcastMessageProc();
                        }
                        comm_manager_->waitForMessageProcesses();
                    }
                    gvt_manager_->workerThreadGVTBarrierSync();
                    prev_gvt = gvt;

                    // Line 18-20
                    gvt_manager_->progressGVT(temp_local_min);

                    if(comm_manager_->getNumProcessesGVT() > 1) {                       
                        comm_manager_->minAllReduceUint(&temp_local_min, &next_gvt);
                        if(prev_gvt < temp_local_min) {
                            gvt_manager_->setPrevGVT(prev_gvt);
                        }
                        
                        gvt_manager_->setGVT(next_gvt);
                    }

                    if (gvt == std::numeric_limits<unsigned int>::max()) {
                        return;
                    }
                    if (gvt_manager_->gvtUpdated()) {
                        gvt = gvt_manager_->getGVT();
                    }
                }	   

            }
}
