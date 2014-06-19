#ifndef TIME_WARP_EVENT_DISPATCHER_H
#define TIME_WARP_EVENT_DISPATCHER_H

#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <functional>   // for std::function

#include "EventDispatcher.hpp"
#include "GVTManager.hpp"
#include "MPICommunicationManager.hpp"

namespace warped {

class LTSFQueue;
class Partitioner;
class SimulationObject;

// This is the EventDispatcher that will run a Time Warp synchronized parallel simulation.

// TODO: This is currently entirely incomplete. At this point, it's just a
// possibility for how the class might be structured in the future. All the
// configurable components should be passed in to the constructor and saved as
// const std::unique_ptrs. Any configuration values should be stored as const
// members. Any mutable members should be stored in thread_local storage or in
// a std::atomic, depending on the member.
class TimeWarpEventDispatcher : public EventDispatcher {
public:
    TimeWarpEventDispatcher(unsigned int max_sim_time,
        std::unique_ptr<LTSFQueue> events, std::unique_ptr<GVTManager> gvt_manager);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

    void getAndDispatchMessages();

private:
    void processEvents();

    const std::unique_ptr<LTSFQueue> events_;
    std::unordered_map<std::string, SimulationObject*> objects_by_name_;

    std::atomic<unsigned int> minimum_local_virtual_time_;

    const std::unique_ptr<GVTManager> gvt_manager_;

    std::function<unsigned int()> getMinimumLVT =
        [=](){ return minimum_local_virtual_time_.load(); };

    const std::unique_ptr<MPICommunicationManager> mpi_manager_;

};

} // namespace warped

#endif
