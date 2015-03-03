#include <limits> // for std::numeric_limits<unsigned int>::max();
#include <cassert>
#include <algorithm> // for std::min

#include "TimeWarpStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

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

unsigned int TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    state_queue_lock_[local_object_id].lock();

    if (state_queue_[local_object_id].empty()) {
        state_queue_lock_[local_object_id].unlock();
        return 0;
    }

    auto min = state_queue_[local_object_id].begin();
    auto next = std::next(min);
    while ((next != state_queue_[local_object_id].end()) && (next->first->timestamp() < gvt)) {
        min = state_queue_[local_object_id].erase(min);
    }

    state_queue_lock_[local_object_id].unlock();

    return min->first->timestamp();
}

std::size_t TimeWarpStateManager::size(unsigned int local_object_id) {
    return state_queue_[local_object_id].size();
}

} // namespace warped
