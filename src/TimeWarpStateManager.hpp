#ifndef STATE_MANAGER_HPP
#define STATE_MANAGER_HPP

/* This class provides an interface for a specific State Manager 
 * that implements a specific algorithm for saving and restoring 
 * the state.
 */

#include <memory>
#include <deque>

#include "TimeWarpEventDispatcher.hpp"
#include "ObjectState.hpp"

namespace warped {

class TimeWarpStateManager {

public:

    TimeWarpStateManager() = default;

    // Creates a state queue for each object as well as a lock for each state queue
    virtual void initialize(unsigned int num_local_objects);

    // Removes states from state queues before or equal to the gvt for the specified object
    unsigned int fossilCollect(unsigned int gvt, unsigned int local_object_id);

    // Restores a state based on rollback time for the given object.
    std::shared_ptr<Event> restoreState(std::shared_ptr<Event> rollback_event, unsigned int local_object_id,
        SimulationObject *object);

    // Number of states in the state queue for the specified object
    std::size_t size(unsigned int local_object_id);

    virtual void saveState(std::shared_ptr<Event> current_event, unsigned int local_object_id,
        SimulationObject *object) = 0;

protected:

    struct SavedState {
        SavedState(std::shared_ptr<Event> state_event, std::unique_ptr<ObjectState> object_state,
            std::shared_ptr<std::stringstream> rng_state)
                : state_event_(state_event), object_state_(std::move(object_state)),
                rng_state_(rng_state) {}

        SavedState& operator=(SavedState&& other) {
            this->state_event_ = other.state_event_;
            this->object_state_ = std::move(other.object_state_);
            this->rng_state_ = other.rng_state_;
            return *this;
        }

        std::shared_ptr<Event> state_event_;
        std::unique_ptr<ObjectState> object_state_;
        std::shared_ptr<std::stringstream> rng_state_;
    };

    // Array of vectors (Array of states queues), one for each object
    std::unique_ptr<std::deque<SavedState> []> state_queue_;

    unsigned int num_local_objects_;
};

} // warped namespace

#endif
