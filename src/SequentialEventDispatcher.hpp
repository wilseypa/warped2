#ifndef SEQUENTIAL_EVENT_DISPATCHER_HPP
#define SEQUENTIAL_EVENT_DISPATCHER_HPP

#include <memory>
#include <vector>

#include "EventDispatcher.hpp"

namespace warped {

class EventStatistics;
class SimulationObject;

// This class is an EventDispatcher that runs on a single thread and process.
class SequentialEventDispatcher : public EventDispatcher {
public:
    SequentialEventDispatcher(unsigned int max_sim_time, std::unique_ptr<EventStatistics> stats);
    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

private:
    std::unique_ptr<EventStatistics> stats_;
};

} // namespace warped

#endif