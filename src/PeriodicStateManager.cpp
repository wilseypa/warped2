#include "PeriodicStateManager.hpp"
#include "SimulationObject.hpp"

namespace warped
{

void PeriodicStateManager::saveState(unsigned int current_time, unsigned int object_id,
    SimulationObject *object) {

    if (count_ == 0) {

        state_queue_lock_[object_id].lock();
        state_queue_[object_id].insert(std::make_pair(current_time, object->getState().clone()));
        state_queue_lock_[object_id].unlock();

        count_[object_id] = period_;
    } else {
        count_[object_id]--;
    }

}

} // namespace warped
