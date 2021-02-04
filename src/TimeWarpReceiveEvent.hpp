// Note this file pair will house the gvtcntrl thread.
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
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp" 
#include <shared_mutex>


namespace warped
{
    
    class RecieveEvent
    {
        public:

            RecieveEvent::RecieveEvent(
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

            
            void RecieveEvent::thread();
            void RecieveEvent::initialize();



            pthread_barrier_t termination_barrier_sync_1;
            pthread_barrier_t termination_barrier_sync_2;
            pthread_barrier_t worker_thread_barrier_sync;

            unsigned int num_worker_threads_;
            bool is_lp_migration_on_;
            unsigned int num_refresh_per_gvt_;
            unsigned int num_events_per_refresh_;
            unsigned int num_local_lps_;

            std::unordered_map<std::string, LogicalProcess*> lps_by_name_;
            std::unordered_map<std::string, unsigned int> local_lp_id_by_name_;
            std::unordered_map<unsigned int, std::string> local_lp_name_by_id_;

        #ifdef TIMEWARP_EVENT_LOG
            // Event log for each worker thread
            std::vector<std::unique_ptr<CircularList<std::string>>> event_log_;
        #endif

            const std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;
            const std::unique_ptr<TimeWarpEventSet> event_set_;
            const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
            const std::unique_ptr<TimeWarpStateManager> state_manager_;
            const std::unique_ptr<TimeWarpOutputManager> output_manager_;
            const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
            const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
            const std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher_;
            const std::unique_ptr<TimeWarpStatistics> tw_stats_;

            static THREAD_LOCAL_SPECIFIER unsigned int thread_id;
    };
}