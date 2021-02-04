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
#include "TimeWarpReceiveEvent.hpp"
#include "TimeWarpOutputManager.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp" 
#include <shared_mutex>

namespace warped
{
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
    std::unique_ptr<TimeWarpStatistics> tw_stats) :
        num_worker_threads_(num_worker_threads),
        is_lp_migration_on_(is_lp_migration_on),
        num_refresh_per_gvt_(num_refresh_per_gvt), num_events_per_refresh_(num_events_per_refresh), 
        comm_manager_(comm_manager), event_set_(std::move(event_set)), 
        gvt_manager_(std::move(gvt_manager)), state_manager_(std::move(state_manager)),
        output_manager_(std::move(output_manager)), twfs_manager_(std::move(twfs_manager)),
        termination_manager_(std::move(termination_manager)), tw_stats_(std::move(tw_stats)) {}



    void RecieveEvent::thread()
    {
        while(1)
        {

        }

    }

}