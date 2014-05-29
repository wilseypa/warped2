#include "TimeWarpEventDispatcher.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Event.hpp"
#include "EventDispatcher.hpp"
#include "LTSFQueue.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"

namespace warped {

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
                                                 std::unique_ptr<LTSFQueue> events)
    : EventDispatcher(max_sim_time), events_(std::move(events)) {}

// This function implementation is merely a speculation for how the this
// function might work in the future. It's definitely incomplete, and may not
// be the best approach at all.
void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {
    for (auto& partition : objects) {
        for (auto& ob : partition) {
            events_->push(ob->createInitialEvents());
            objects_by_name_[ob->name_] = ob;
        }
    }

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
        threads.push_back(std::thread {&TimeWarpEventDispatcher::processEvents, this});
    }

    processEvents();

    for (auto& t : threads) {
        t.join();
    }

}

void TimeWarpEventDispatcher::processEvents() {
    while (current_sim_time_.load() < max_sim_time_ && !events_->empty()) {
        // handle the events
    }
}

} // namespace warped