#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#include <limits>
#include <vector>

namespace warped {

class SimulationObject;

// An EventDispatcher is responsible for sending events between objects.
class EventDispatcher {
public:
    // if max_sim_time is greater than 0, events will be processed until the
    // timestamp of all remaining events is greater than max_sim_time.
    // Otherwise, events will be processed until none remain.
    EventDispatcher(unsigned int max_sim_time)
        : max_sim_time_(max_sim_time > 0 ?
                        max_sim_time : std::numeric_limits<unsigned int>::max()) {}
    virtual ~EventDispatcher() {}

    // This method will call createInitialEvents() on all objects, then
    // process events until a termination condition is reached.
    // The objects argument is a partitioned set of SimulationObjects
    virtual void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects) = 0;

protected:
    const unsigned int max_sim_time_;
};

} // namespace warped

#endif