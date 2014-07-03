#ifndef PERIODIC_STATE_MANAGER_HPP
#define PERIODIC_STATE_MANAGER_HPP

#include <cstring>

#include "StateManager.hpp"

namespace warped {

class PeriodicStateManager : public StateManager {
public:

    PeriodicStateManager(unsigned int num_objects, unsigned int period) :
        StateManager(num_objects), period_(period),
        count_(make_unique<unsigned int []>(num_objects)) {

        // Initialize count_ values for all objects to zero
        memset(count_.get(), 0, sizeof(unsigned int)*num_objects);
    }

    virtual void saveState(unsigned int current_time, unsigned int local_object_id,
        SimulationObject *object);

private:
    // Period is the number of events that must be processed before saving state
    unsigned int period_;

    // Count keeps a running count of the number of event that we must wait before saving
    // the state again (per object)
    std::unique_ptr<unsigned int []> count_;
};

} // namespace warped

#endif
