#include "TimeWarpPeriodicStateManager.hpp"
#include "LogicalProcess.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpPeriodicStateManager::initialize(unsigned int num_local_lps) {

    // Initialize counts to zero
    count_ = make_unique<unsigned int []>(num_local_lps);
    memset(count_.get(), 0, num_local_lps*sizeof(unsigned int));

    TimeWarpStateManager::initialize(num_local_lps);
}

void TimeWarpPeriodicStateManager::saveState(std::shared_ptr<Event> current_event,
    unsigned int local_lp_id, LogicalProcess *lp) {

    // Save if count is zero. State will always be saved on first call
    if (count_[local_lp_id] == 0) {

        auto lp_state = lp->getState().clone();

        std::list<std::shared_ptr<std::stringstream> > saved_rng_list;
        for (auto rng = lp->rng_list_.begin(); rng != lp->rng_list_.end(); rng++) {
            auto ss = std::make_shared<std::stringstream>();
            (*rng)->getState(*ss);
            saved_rng_list.push_back(ss);
        }

        state_queue_[local_lp_id].emplace_back(current_event, std::move(lp_state), saved_rng_list);

        count_[local_lp_id] = period_ - 1;
    } else {
        count_[local_lp_id]--;
    }

}

} // namespace warped
