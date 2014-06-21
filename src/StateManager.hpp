#ifndef STATE_MANAGER_HPP
#define STATE_MANAGER_HPP

/* This class provides an interface for a specific State Manager 
 * that implements a specific algorithm for saving and restoring 
 * the state.
 */

namespace warped {

class StateManager {

public:

    StateManager();

    ~StateManager();

    virtual unsigned int restoreState( unsigned int rollbackTime, SimulationObject *object);

    virtual void saveState( unsigned int currentTime, SimulationObject *object);

};

} // warped namespace

#endif
