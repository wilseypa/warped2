#include <limits> // for std::numeric_limits<unsigned int>::max();
#include <cassert>

#include "TimeWarpStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpStateManager::initialize(unsigned int num_local_objects) {
    state_queue_ = make_unique<std::vector<std::pair<unsigned int, std::unique_ptr<ObjectState>>>[]>
        (num_local_objects);

    state_queue_lock_ = make_unique<std::mutex []>(num_local_objects);

}

unsigned int TimeWarpStateManager::restoreState(unsigned int rollback_time,
    unsigned int local_object_id, SimulationObject *object) {

    state_queue_lock_[local_object_id].lock();

    assert(!state_queue_[local_object_id].empty());

    auto max = std::prev(state_queue_[local_object_id].end());
    while ((max != state_queue_[local_object_id].begin()) && (max->first >= rollback_time)) {
        max->second.reset();
        state_queue_[local_object_id].erase(max);
        max = std::prev(state_queue_[local_object_id].end());
    }

    state_queue_lock_[local_object_id].unlock();

    object->getState().restoreState(*max->second);

    return max->first;
}

unsigned int TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    unsigned int retval = std::numeric_limits<unsigned int>::max();
    state_queue_lock_[local_object_id].lock();

    auto min = state_queue_[local_object_id].begin();
    while (min->first < gvt && min != state_queue_[local_object_id].end()) {
        min->second.reset();
        min = state_queue_[local_object_id].erase(min);
    }

    if (min != state_queue_[local_object_id].end()) {
        retval = min->first;
    }

    state_queue_lock_[local_object_id].unlock();

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
