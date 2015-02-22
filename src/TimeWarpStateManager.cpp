#include <limits> // for std::numeric_limits<unsigned int>::max();
#include <cassert>

#include "TimeWarpStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpStateManager::initialize(unsigned int num_local_objects) {
   state_queue_ = make_unique<std::vector<std::pair<std::shared_ptr<Event>, std::unique_ptr<ObjectState>>>[]>
        (num_local_objects);

    state_queue_lock_ = make_unique<std::mutex []>(num_local_objects);

    num_local_objects_ = num_local_objects;
}

std::shared_ptr<Event> TimeWarpStateManager::restoreState(std::shared_ptr<Event> rollback_event,
    unsigned int local_object_id, SimulationObject *object) {

    state_queue_lock_[local_object_id].lock();

    assert(!state_queue_[local_object_id].empty());

    auto max = std::prev(state_queue_[local_object_id].end());
    while ((max != state_queue_[local_object_id].begin()) && (*rollback_event <= *max->first)) {
        state_queue_[local_object_id].erase(max);
        max = std::prev(state_queue_[local_object_id].end());
    }

    state_queue_lock_[local_object_id].unlock();

    object->getState().restoreState(*max->second);

    return max->first;
}

void TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    state_queue_lock_[local_object_id].lock();

    auto min = state_queue_[local_object_id].begin();
    while (min != std::prev(state_queue_[local_object_id].end()) && min->first->timestamp() < gvt) {
        min = state_queue_[local_object_id].erase(min);
    }

    state_queue_lock_[local_object_id].unlock();
}

void TimeWarpStateManager::fossilCollectAll(unsigned int gvt) {
    for (unsigned int i = 0; i < num_local_objects_; i++) {
        fossilCollect(gvt, i);
    }
}

std::size_t TimeWarpStateManager::size(unsigned int local_object_id) {
    return state_queue_[local_object_id].size();
}

} // namespace warped
