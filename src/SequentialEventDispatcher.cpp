#include "SequentialEventDispatcher.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "EventStatistics.hpp"
#include "SimulationObject.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

SequentialEventDispatcher::SequentialEventDispatcher(unsigned int max_sim_time,
                                                     std::unique_ptr<EventStatistics> stats)
    : EventDispatcher(max_sim_time), stats_(std::move(stats)) {}

void SequentialEventDispatcher::startSimulation(
    const std::vector<std::vector<SimulationObject*>>& objects) {
    if (objects.size() != 1) {
        throw std::runtime_error(std::string("Sequential simulation only supports 1 partition."));
    }

    std::unordered_map<std::string, SimulationObject*> objects_by_name;
    STLLTSFQueue events;
    unsigned int current_sim_time = 0;

    for (auto& ob : objects[0]) {
        auto new_events = ob->createInitialEvents();
        stats_->record(ob->name_, current_sim_time, new_events);
        events.push(std::move(new_events));
        objects_by_name[ob->name_] = ob;
    }

    while (current_sim_time < max_sim_time_ && !events.empty()) {
        auto event = events.pop();
        current_sim_time = event->timestamp();
        auto receiver = objects_by_name[event->receiverName()];
        auto new_events = receiver->receiveEvent(*event.get());
        stats_->record(event->receiverName(), current_sim_time, new_events);
        events.push(std::move(new_events));
    }

    stats_->writeToFile();
}

} // namespace warped