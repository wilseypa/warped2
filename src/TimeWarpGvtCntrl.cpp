#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <limits>       // for std::numeric_limits<>::max();
#include <algorithm>    // for std::min
#include <chrono>       // for std::chrono::steady_clock
#include <iostream>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "LogicalProcess.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpGVTManager.hpp"
#include "TimeWarpStateManager.hpp"
#include "TimeWarpOutputManager.hpp"
#include "TimeWarpGvtCntrl.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"   


namespace warped
{

        GvtCntrl::GvtCntrl(
            std::shared_ptr<TimeWarpCommunicationManager> comm_manager,
            std::unique_ptr<TimeWarpEventSet> event_set,
            std::unique_ptr<TimeWarpGVTManager> gvt_manager,
            std::unique_ptr<TimeWarpStateManager> state_manager,
            std::unique_ptr<TimeWarpOutputManager> output_manager,
            std::unique_ptr<TimeWarpFileStreamManager> twfs_manager,
            std::unique_ptr<TimeWarpTerminationManager> termination_manager,
            std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher,
            std::unique_ptr<TimeWarpStatistics> tw_stats) :
                comm_manager_(comm_manager), event_set_(std::move(event_set)), 
                gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
                output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
                termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}

            void GvtCntrl::initialize()
            {

            }

            void GvtCntrl::thread()
            {                
                gvt = gvt_manager_->getGVT();

                // These will need passed to the housekeeping class and passed into the constructor so they can be shared by the threads.
                
                

                // Will run until this thread is destroyed
                while(!termination_manager_->terminationStatus()) {

                    if(comm_manager_->getID() == 0) {
                        sleep(gvt_manager_->getGVTPeriod());
                    }
                    
                    if(comm_manager_->getNumProcesses() > 1) {
                        comm_manager_->waitForGVT();
                    }
                    
                    gvt_manager_->setGVTEstCycle(true);

                    // This aligns with line 9 of the algorithm loop.
                    gvt_manager_->workerThreadGVTBarrierSync();
                    if(comm_manager_->getNumProcesses() > 1){
                        if(comm_manager_->getID() == 0){
                            // Move to comm manager.
                            char* message = "Verify_Idle";
                            MPI_Bcast(message, strlen(message)+1, MPI_CHAR, 0, MPI_COMM_WORLD);
                        }
                        comm_manager_->waitForMessageProcesses();
                    }
                    gvt_manager_->workerThreadGVTBarrierSync();
                    prev_gvt = gvt;

                    // Line 18-20
                    gvt_manager_->progressGVT(temp_local_min);

                    if(comm_manager_->getNumProcesses() > 1) {                       
                        comm_manager_->minAllReduceUint(&temp_local_min, &next_gvt);
                        if(prev_gvt < temp_local_min) {
                            gvt_manager_->setPrevGVT(prev_gvt);
                        }
                        
                        gvt_manager_->setGVT(next_gvt);
                    }

                    if (gvt == std::numeric_limits<unsigned int>::max()) {

                        // Line 26.
                        MPI_Finalize();
                        exit(EXIT_SUCCESS);
                    }
                    if (gvt_manager_->gvtUpdated()) {
                        gvt = gvt_manager_->getGVT();
                    }
                }	   

            }
}
