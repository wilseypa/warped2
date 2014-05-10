#include "SequentialEventDispatcher.hpp"

#include <utility>
#include <vector>

#include "Event.hpp"
#include "SimulationObject.hpp"

namespace warped {

SequentialEventDispatcher::SequentialEventDispatcher(unsigned int max_sim_time)
    : EventDispatcher(max_sim_time), objects_(), events_() {}

void SequentialEventDispatcher::startSimulation(
    std::vector<std::unique_ptr<SimulationObject>>& objects) {
    for (auto& ob : objects) {
        events_.push(ob->createInitialEvents());
        objects_[ob->name_] = std::move(ob);
    }

    while(current_sim_time_ < max_sim_time_ && !events_.empty()) {
        auto event = events_.pop();
        current_sim_time_ = event->get_receive_time();
        auto receiver = objects_[event->get_receiver_name()].get();
        events_.push(receiver->receiveEvent(event.get()));
    }
}

} // namespace warped