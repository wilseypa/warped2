#ifndef PERIODIC_STATE_MANAGER_HPP
#define PERIODIC_STATE_MANAGER_HPP

#include <cstring>

#include "StateManager.hpp"

namespace warped {

class PeriodicStateManager : public StateManager {
public:

    PeriodicStateManager(unsigned int period) :
        period_(period) {}

    void initialize(unsigned int num_local_objects) override;

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
