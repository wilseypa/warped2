#ifndef TIME_WARP_EVENT_DISPATCHER_H
#define TIME_WARP_EVENT_DISPATCHER_H

#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "EventDispatcher.hpp"

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
                            std::unique_ptr<LTSFQueue> events);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

private:
    void processEvents();

    std::atomic<unsigned int> current_sim_time_;
    const std::unique_ptr<LTSFQueue> events_;
    std::unordered_map<std::string, SimulationObject*> objects_by_name_;
};

} // namespace warped

#endif