#include "SequentialEventDispatcher.hpp"

#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Event.hpp"
#include "SimulationObject.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

SequentialEventDispatcher::SequentialEventDispatcher(unsigned int max_sim_time)
    : EventDispatcher(max_sim_time) {}

void SequentialEventDispatcher::startSimulation(
    const std::vector<std::vector<SimulationObject*>>& objects) {
    if (objects.size() != 1) {
        throw std::runtime_error(std::string("Sequention simualtion only supports 1 partition."));
    }
    
    std::unordered_map<std::string, SimulationObject*> objects_by_name;
    STLLTSFQueue events;
    unsigned int current_sim_time = 0;

    for (auto& ob : objects[0]) {
        events.push(ob->createInitialEvents());
        objects_by_name[ob->name_] = ob;
    }

    while (current_sim_time < max_sim_time_ && !events.empty()) {
        auto event = events.pop();
        current_sim_time = event->timestamp();
        auto receiver = objects_by_name[event->receiverName()];
        events.push(receiver->receiveEvent(*event.get()));
    }
}

} // namespace warped