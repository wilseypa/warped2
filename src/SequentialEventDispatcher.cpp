#include "SequentialEventDispatcher.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

#include "Event.hpp"
#include "SimulationObject.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

SequentialEventDispatcher::SequentialEventDispatcher(unsigned int max_sim_time)
    : EventDispatcher(max_sim_time) {}

void SequentialEventDispatcher::startSimulation(const std::vector<SimulationObject*>& objects) {
    std::unordered_map<std::string, SimulationObject*> objects_by_name;
    STLLTSFQueue events;
    unsigned int current_sim_time = 0;

    for (auto& ob : objects) {
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