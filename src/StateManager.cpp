#include "StateManager.hpp"
#include "SimulationObject.hpp"

namespace warped {

unsigned int StateManager::restoreState(unsigned int rollback_time, unsigned int object_id,
    SimulationObject *object) {

    state_queue_lock_[object_id].lock();

    auto max = std::prev(state_queue_[object_id].end());
    while (max->first > rollback_time) {
        state_queue_[object_id].erase(max--);
    }

    state_queue_lock_[object_id].unlock();

    object->getState() = *(max->second);

    return rollback_time;
}

void StateManager::fossilCollect(unsigned int gvt, unsigned int object_id) {

    state_queue_lock_[object_id].lock();

    auto min = state_queue_[object_id].begin();
    while (min->first < gvt) {
        state_queue_[object_id].erase(min++);
    }

    state_queue_lock_[object_id].unlock();

}

} // namespace warped
