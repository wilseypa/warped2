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

        // Copy state and save it to state vector for object with local_object_id
        state_queue_[local_object_id].push_back(std::make_pair(current_event,
            object->getState().clone()));

        count_[local_object_id] = period_ - 1;
    } else {
        count_[local_object_id]--;
    }

}

} // namespace warped
