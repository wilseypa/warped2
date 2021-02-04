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

            void GvtCntrl::initialize()
            {

            }

            void GvtCntrl::thread()
            {                
                unsigned int gvt = 0;
                unsigned int temp_local_min;
                unsigned int next_gvt;

                // These will need passed to the housekeeping class and passed into the constructor so they can be shapred by the threads.
                MPI_Comm GVT = MPI_Comm(); 
                MPI_Comm MSGS_PROC = MPI_Comm();
                

                // Will run until this thread is destroyed
                while(!termination_manager_->terminationStatus()){
                    while(1){           
                        comm_manager_->handleMessages();
                        
                    // Report GVT Flag is updated whenever a message is recieved. Look at receiveGVTSynchTrigger() in TWSynchronousGVTManager
                        
                        if (gvt_manager_->getGVTFlag()){
                            if (comm_manager_->getID() == 0){
                                while (!comm_manager_->getTokenSendConfirmation()){
                                    comm_manager_->handleMessages();
                                }
                                comm_manager_->setTokenSendConfirmation(false);
                                break;
                            } else {
                                break;
                            }
                        }
                    }

                    if(comm_manager_->getID() == 0){
                        sleep(gvt);
                    }
                    
                    if(comm_manager_->getNumProcesses() > 1){
                        MPI_Barrier(GVT);
                    }

                    // This aligns with line 9 of the algorithm loop.
                    gvt_manager_->workerThreadGVTBarrierSync();
                    if(comm_manager_->getNumProcesses() > 1){
                        if(comm_manager_->getID() == 0){
                            char* message = "Verify_Idle";
                            MPI_Bcast(message, strlen(message)+1, MPI_CHAR, 0, MPI_COMM_WORLD);
                        }
                        MPI_Barrier(MSGS_PROC);
                    }

                    // Not sure what this is used for yet.
                    /*host_node_done_lock_.lock();
                    host_node_done_ = false;
                    host_node_done_lock_.unlock();*/
                    
                    // Line 16-19
                    gvt_manager_->progressGVT(temp_local_min);

                    if(comm_manager_->getNumProcesses() > 1){
                        next_gvt = gvt_manager_->getNextGVT();
                        comm_manager_->minAllReduceUint(&temp_local_min, &next_gvt);
                        gvt_manager_->setNextGVT(next_gvt);
                    }

                    if (gvt == std::numeric_limits<unsigned int>::max()){
                        /*terminate_GVT_timer_lock_.lock();
                        terminate_GVT_timer_ = true;
                        terminate_GVT_timer_lock_.unlock();*/
                        // Still need to add line 25.

                        // Line 26.
                        MPI_Finalize();
                        exit(EXIT_SUCCESS);
                        break;
                    }
                    if (gvt_manager_->gvtUpdated()) {
                        gvt = gvt_manager_->getGVT();
                    }
                }	   

            }
}