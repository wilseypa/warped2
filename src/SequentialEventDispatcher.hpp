#ifndef SEQUENTIAL_EVENT_DISPATCHER_H
#define SEQUENTIAL_EVENT_DISPATCHER_H

#include <vector>

#include "EventDispatcher.hpp"

namespace warped {

class SimulationObject;

// This class is an EventDispatcher that runs on a single thread and process.
class SequentialEventDispatcher : public EventDispatcher {
public:
    SequentialEventDispatcher(unsigned int max_sim_time);
    void startSimulation(std::vector<SimulationObject*>& objects);
};

} // namespace warped

#endif