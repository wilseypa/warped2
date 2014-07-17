#include <limits> // for std::numeric_limits<unsigned int>::max();

#include "TimeWarpStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpStateManager::initialize(unsigned int num_local_objects) {
    state_queue_ = make_unique<std::multimap<unsigned int, std::unique_ptr<ObjectState>> []>
        (num_local_objects);

    state_queue_lock_ = make_unique<std::mutex []>(num_local_objects);

}

// TODO this function assumes right now that there will always be a valid state to restore
unsigned int TimeWarpStateManager::restoreState(unsigned int rollback_time,
    unsigned int local_object_id, SimulationObject *object) {

    state_queue_lock_[local_object_id].lock();

    auto max = std::prev(state_queue_[local_object_id].end());
    while (max->first > rollback_time) {
        state_queue_[local_object_id].erase(max--);
    }

    state_queue_lock_[local_object_id].unlock();

    object->getState().restoreState(*max->second);

    return max->first;
}

unsigned int TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    unsigned int retval = std::numeric_limits<unsigned int>::max();
    state_queue_lock_[local_object_id].lock();

    auto min = state_queue_[local_object_id].begin();
    while (min->first <= gvt && min != state_queue_[local_object_id].end()) {
        state_queue_[local_object_id].erase(min++);
    }

    state_queue_lock_[local_object_id].unlock();

    if (min != state_queue_[local_object_id].end()) {
        retval = min->first;
    }

    return retval;
}

void TimeWarpStateManager::fossilCollectAll(unsigned int gvt) {
    for (unsigned int i = 0; i < state_queue_.get()->size(); i++) {
        fossilCollect(gvt, i);
    }
}

std::size_t TimeWarpStateManager::size(unsigned int local_object_id) {
    return state_queue_[local_object_id].size();
}

} // namespace warped
