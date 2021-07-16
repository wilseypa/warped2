/*
--------- old receive.hpp

class RecieveEvent
    {
        public:

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

            static THREAD_LOCAL_SPECIFIER unsigned int thread_id;
    };
*/

#include "HouseKeeping.hpp"
//#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpStatistics.hpp"

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

            void receiveEvent::thread(std::unique_ptr<TimeWarpKernelMessage> kmsg);
            
        private:

            const std::shared_ptr<TimeWarpMPICommunicationManager> comm_manager_;
            const std::unique_ptr<TimeWarpEventSet> event_set_;
            const std::unique_ptr<TimeWarpGVTManager> gvt_manager_;
            const std::unique_ptr<TimeWarpStateManager> state_manager_;
            const std::unique_ptr<TimeWarpOutputManager> output_manager_;
            const std::unique_ptr<TimeWarpFileStreamManager> twfs_manager_;
            const std::unique_ptr<TimeWarpTerminationManager> termination_manager_;
            const std::unique_ptr<TimeWarpEventDispatcher> event_dispatcher_;
            const std::unique_ptr<TimeWarpStatistics> tw_stats_;

    };
}