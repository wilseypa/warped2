#ifndef PERIODIC_STATE_MANAGER_HPP
#define PERIODIC_STATE_MANAGER_HPP

#include "StateManager.hpp"

namespace warped {

class PeriodicStateManager : public StateManager {
public:

    virtual void saveState(unsigned int current_time, unsigned int object_id);

};

} // namespace warped

#endif
