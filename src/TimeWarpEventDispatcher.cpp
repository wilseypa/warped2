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

// This is just to ensure that it compiles correctly
#include "EventMessage.hpp"

namespace warped {

TimeWarpEventDispatcher::TimeWarpEventDispatcher(unsigned int max_sim_time,
    std::unique_ptr<LTSFQueue> events ,std::unique_ptr<GVTManager> gvt_manager)
    : EventDispatcher(max_sim_time), events_(std::move(events)),
      gvt_manager_(std::move(gvt_manager)) {}

// This function implementation is merely a speculation for how the this
// function might work in the future. It's definitely incomplete, and may not
// be the best approach at all.
void TimeWarpEventDispatcher::startSimulation(const std::vector<std::vector<SimulationObject*>>&
                                              objects) {

    mpi_manager_->initialize();

    for (auto& partition : objects) {
        for (auto& ob : partition) {
            events_->push(ob->createInitialEvents());
            objects_by_name_[ob->name_] = ob;
        }
    }

    // Create worker threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
        threads.push_back(std::thread {&TimeWarpEventDispatcher::processEvents, this});
    }

    // Master thread main loop
    while (gvt_manager_->getGVT() < max_sim_time_) {
        if (mpi_manager_->getID() == 0) {
            gvt_manager_->calculateGVT(getMinimumLVT, mpi_manager_.get());
        }

        mpi_manager_->sendAllMessages();
    }

    for (auto& t : threads) {
        t.join();
    }

    mpi_manager_->finalize();
}

void TimeWarpEventDispatcher::processEvents() {

}

} // namespace warped
