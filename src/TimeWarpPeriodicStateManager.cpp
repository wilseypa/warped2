#include "TimeWarpPeriodicStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpPeriodicStateManager::initialize(unsigned int num_local_objects) {

    // Initialize counts to zero
    count_ = make_unique<unsigned int []>(num_local_objects);
    memset(count_.get(), 0, num_local_objects*sizeof(unsigned int));

    TimeWarpStateManager::initialize(num_local_objects);
}

void TimeWarpPeriodicStateManager::saveState(std::shared_ptr<Event> current_event,
    unsigned int local_object_id, SimulationObject *object) {

    // Save if count is zero. State will always be saved on first call
    if (count_[local_object_id] == 0) {

        auto object_state = object->getState().clone();

        std::list<std::shared_ptr<std::stringstream> > saved_rng_list;
        for (auto rng = object->rng_list_.begin(); rng != object->rng_list_.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_rng_list.push_back(ss);
        }

        state_queue_[local_object_id].emplace_back(current_event, std::move(object_state), saved_rng_list);

        count_[local_object_id] = period_ - 1;
    } else {
        count_[local_object_id]--;
    }

}

} // namespace warped
