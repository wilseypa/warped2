#ifndef STATE_MANAGER_HPP
#define STATE_MANAGER_HPP

/* This class provides an interface for a specific State Manager 
 * that implements a specific algorithm for saving and restoring 
 * the state.
 */

#include <memory>
#include <deque>
#include <list>

#include "TimeWarpEventDispatcher.hpp"
#include "LogicalProcess.hpp"
#include "LPState.hpp"

namespace warped {

class TimeWarpStateManager {
public:

    TimeWarpStateManager() = default;

    // Creates a state queue for each lps as well as a lock for each state queue
    virtual void initialize(unsigned int num_local_lps);

    // Removes states from state queues before or equal to the gvt for the specified lp
    unsigned int fossilCollect(unsigned int gvt, unsigned int local_lp_id);

    // Restores a state based on rollback time for the given lp.
    std::shared_ptr<Event> restoreState(std::shared_ptr<Event> rollback_event, unsigned int local_lp_id,
        LogicalProcess *lp);

    // Number of states in the state queue for the specified lp
    std::size_t size(unsigned int local_lp_id);

    virtual void saveState(std::shared_ptr<Event> current_event, unsigned int local_lp_id,
        LogicalProcess *lp) = 0;

protected:

    struct SavedState {
        SavedState(std::shared_ptr<Event> state_event, std::unique_ptr<LPState> lp_state,
            std::list<std::shared_ptr<std::stringstream> > rng_state)
                : state_event_(state_event), lp_state_(std::move(lp_state)),
                rng_state_(rng_state) {}

        std::shared_ptr<Event> state_event_;
        std::unique_ptr<LPState> lp_state_;
        std::list<std::shared_ptr<std::stringstream> > rng_state_;
    };

    // Array of vectors (Array of states queues), one for each lp
    std::unique_ptr<std::deque<SavedState> []> state_queue_;

    unsigned int num_local_lps_;
};

} // warped namespace

#endif
