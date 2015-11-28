#include <limits> // for std::numeric_limits<unsigned int>::max();
#include <cassert>
#include <algorithm> // for std::min

#include "TimeWarpStateManager.hpp"
#include "LogicalProcess.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

namespace warped {

void TimeWarpStateManager::initialize(unsigned int num_local_lps) {
    state_queue_ = make_unique<std::deque<SavedState>[]>(num_local_lps);
    num_local_lps_ = num_local_lps;
}

std::shared_ptr<Event> TimeWarpStateManager::restoreState(std::shared_ptr<Event> rollback_event,
    unsigned int local_lp_id, LogicalProcess *lp) {

    assert(!state_queue_[local_lp_id].empty());

    auto max = state_queue_[local_lp_id].rbegin();
    // NOTE: state_queue_ for each lp is a vector of pairs.
    //  Each pair consists of a state and an event that last contributed to that state.
    //      first == the event
    //      second == the state

    while ((max != state_queue_[local_lp_id].rend()) && (*max->state_event_ >= *rollback_event)) {
        state_queue_[local_lp_id].pop_back();
        max = state_queue_[local_lp_id].rbegin();
    }

    // We must have a state that we can go back to.
    assert(max != state_queue_[local_lp_id].rend());

    lp->getState().restoreState(*max->lp_state_);

    // Restore state of random number generators
    for (auto rng = lp->rng_list_.rbegin(); rng != lp->rng_list_.rend(); rng++) {
        auto ss = std::move(max->rng_state_.back());
        max->rng_state_.pop_back();
        (*rng)->restoreState(*ss);
    }

    // Need to resave state of random number generator
    for (auto rng = lp->rng_list_.begin(); rng != lp->rng_list_.end(); rng++) {
        auto ss = std::make_shared<std::stringstream>();
        (*rng)->getState(*ss);
        max->rng_state_.push_back(ss);
    }

    // Return the state
    return max->state_event_;
}

// NOTE: Returns the time at which events should be fossil collected before
unsigned int TimeWarpStateManager::fossilCollect(unsigned int gvt, unsigned int local_lp_id) {

    if (state_queue_[local_lp_id].empty()) {
        // Return zero so that no events are fossil collected.
        return 0;
    }

    // Keep two iterators which keep track of the two minimum elements in the state queue
    auto min = state_queue_[local_lp_id].begin();
    auto next = std::next(min);

    // We need to keep a state that is less than the GVT
    // Loop through and delete states until min timestamp is less than GVT and next timestamp is
    //  greater than GVT.
    while ((next != state_queue_[local_lp_id].end()) && (next->state_event_->timestamp() < gvt)) {
        state_queue_[local_lp_id].pop_front();
        min = state_queue_[local_lp_id].begin();
        next = std::next(min);
    }

    if (gvt == (unsigned int)-1) {
        state_queue_[local_lp_id].pop_front();
        return gvt;
    }

    return min->state_event_->timestamp();
}

// NOTE: Used for debugging
std::size_t TimeWarpStateManager::size(unsigned int local_lp_id) {
    return state_queue_[local_lp_id].size();
}

} // namespace warped
