#ifndef SEQUENTIAL_EVENT_DISPATCHER_H
#define SEQUENTIAL_EVENT_DISPATCHER_H

#include <unordered_map>
#include <string>

#include "EventDispatcher.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

class SequentialEventDispatcher : public EventDispatcher {
public:
    SequentialEventDispatcher(unsigned int max_sim_time);
    void startSimulation(std::vector<std::unique_ptr<SimulationObject>>& objects);

private:
    std::unordered_map<std::string, std::unique_ptr<SimulationObject>> objects_;
    STLLTSFQueue events_;
};

} // namespace warped

#endif