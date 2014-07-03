#include "PeriodicStateManager.hpp"
#include "SimulationObject.hpp"

namespace warped
{

void PeriodicStateManager::saveState(unsigned int current_time, unsigned int local_object_id,
    SimulationObject *object) {

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
