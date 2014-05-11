#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include <limits>

#include "SimulationObject.hpp"

namespace warped {

class EventDispatcher {
public:
    EventDispatcher(unsigned int max_sim_time)
        : max_sim_time_(max_sim_time > 0 ? max_sim_time : std::numeric_limits<unsigned int>::max()),
          current_sim_time_(0) {}
    virtual ~EventDispatcher() {}
    virtual void startSimulation(std::vector<SimulationObject*>& objects) = 0;

protected:
    const unsigned int max_sim_time_;
    unsigned int current_sim_time_;
};

} // namespace warped

#endif