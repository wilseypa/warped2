#ifndef PERIODIC_STATE_MANAGER_HPP
#define PERIODIC_STATE_MANAGER_HPP

#include <cstring>

#include "TimeWarpStateManager.hpp"

/* Subclass of TimeWarpStateManager which implements a periodic state saving. In this
 * implementation, the period is fixed and is a number of events. The save state method
 * should be called for each event processed so that the period count can be decremented. When
 * period count reaches 0, the state is saved.
 */

namespace warped {

class TimeWarpPeriodicStateManager : public TimeWarpStateManager {
public:

    TimeWarpPeriodicStateManager(unsigned int period) :
        period_(period) {}

    void initialize(unsigned int num_local_objects) override;

    // Saves the state of the specified object if the count is equal to 0.
    virtual void saveState(std::shared_ptr<Event> current_event, unsigned int local_object_id,
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
