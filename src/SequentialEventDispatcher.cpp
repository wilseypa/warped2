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

void SequentialEventDispatcher::startSimulation(std::vector<SimulationObject*>& objects) {
    std::unordered_map<std::string, SimulationObject*> objects_;
    STLLTSFQueue events_;

    for (auto& ob : objects) {
        events_.push(ob->createInitialEvents());
        objects_[ob->name_] = ob;
    }

    while (current_sim_time_ < max_sim_time_ && !events_.empty()) {
        auto event = events_.pop();
        current_sim_time_ = event->timestamp();
        auto receiver = objects_[event->receiverName()];
        events_.push(receiver->receiveEvent(*event.get()));
    }
}

} // namespace warped