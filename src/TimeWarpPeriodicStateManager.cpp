#include "TimeWarpPeriodicStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpPeriodicStateManager::initialize(unsigned int num_local_objects) {
    count_ = make_unique<unsigned int []>(num_local_objects);
    memset(count_.get(), 0, num_local_objects*sizeof(unsigned int));
    TimeWarpStateManager::initialize(num_local_objects);
}

void TimeWarpPeriodicStateManager::saveState(unsigned int current_time,
    unsigned int local_object_id, SimulationObject *object) {

    if (count_[local_object_id] == 0) {

        state_queue_lock_[local_object_id].lock();
        state_queue_[local_object_id].insert(std::make_pair(current_time,
            object->getState().clone()));
        state_queue_lock_[local_object_id].unlock();

        count_[local_object_id] = period_ - 1;
    } else {
        count_[local_object_id]--;
    }

}

} // namespace warped
