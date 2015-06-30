#include <limits> // for std::numeric_limits<unsigned int>::max();
#include <cassert>
#include <algorithm> // for std::min

#include "TimeWarpStateManager.hpp"
#include "SimulationObject.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpStateManager::initialize(unsigned int num_local_objects) {
    state_queue_ = make_unique<std::deque<SavedState>[]>(num_local_objects);
    num_local_objects_ = num_local_objects;
}

std::shared_ptr<Event> TimeWarpStateManager::restoreState(std::shared_ptr<Event> rollback_event,
    unsigned int local_object_id, SimulationObject *object) {

    assert(!state_queue_[local_object_id].empty());

    auto max = state_queue_[local_object_id].rbegin();
    // NOTE: state_queue_ for each object is a vector of pairs.
    //  Each pair consists of a state and an event that last contributed to that state.
    //      first == the event
    //      second == the state

    while ((max != state_queue_[local_object_id].rend()) && (*max->state_event_ >= *rollback_event)) {
        state_queue_[local_object_id].pop_back();
        max = state_queue_[local_object_id].rbegin();
    }

    // We must have a state that we can go back to.
    assert(max != state_queue_[local_object_id].rend());

    object->getState().restoreState(*max->object_state_);

    // Restore state of random number generators
    for (auto rng = object->rng_list_.rbegin(); rng != object->rng_list_.rend(); rng++) {
        (*rng)->restoreState(*max->rng_state_);
    }

    // Need to resave state of random number generator
    max->rng_state_ = std::make_shared<std::stringstream>();
    for (auto rng = object->rng_list_.begin(); rng != object->rng_list_.end(); rng++) {
        (*rng)->getState(*max->rng_state_);
    }

    // Return the state
    return max->state_event_;
}

// NOTE: Returns the time at which events should be fossil collected before
unsigned int TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {

    if (state_queue_[local_object_id].empty()) {
        // Return zero so that no events are fossil collected.
        return 0;
    }

    // Keep two iterators which keep track of the two minimum elements in the state queue
    auto min = state_queue_[local_object_id].begin();
    auto next = std::next(min);

    // We need to keep a state that is less than the GVT
    // Loop through and delete states until min timestamp is less than GVT and next timestamp is
    //  greater than GVT.
    while ((next != state_queue_[local_object_id].end()) && (next->state_event_->timestamp() < gvt)) {
        state_queue_[local_object_id].pop_front();
        min = state_queue_[local_object_id].begin();
        next = std::next(min);
    }

    if (gvt == (unsigned int)-1) {
        state_queue_[local_object_id].pop_front();
        return gvt;
    }

    return min->state_event_->timestamp();
}

// NOTE: Used for debugging
std::size_t TimeWarpStateManager::size(unsigned int local_object_id) {
    return state_queue_[local_object_id].size();
}

} // namespace warped
