#ifndef STATE_MANAGER_HPP
#define STATE_MANAGER_HPP

/* This class provides an interface for a specific State Manager 
 * that implements a specific algorithm for saving and restoring 
 * the state.
 */

#include <map>
#include <memory>
#include <mutex>

#include "TimeWarpEventDispatcher.hpp"
#include "ObjectState.hpp"
#include "utility/memory.hpp"

namespace warped {

class StateManager {

public:

    StateManager(TimeWarpEventDispatcher *event_dispatcher) :
        event_dispatcher_(event_dispatcher),
        state_queue_(make_unique<std::multimap<unsigned int, std::unique_ptr<ObjectState>> []>
            (event_dispatcher_->getNumObjects())),
        state_queue_lock_(make_unique<std::mutex []>(event_dispatcher_->getNumObjects()))
        {}

    void fossilCollect(unsigned int gvt, unsigned int object_id);

    unsigned int restoreState(unsigned int rollback_time, unsigned int object_id,
        SimulationObject *object);

    virtual void saveState(unsigned int current_time, unsigned int object_id,
        SimulationObject *object) = 0;

protected:

    TimeWarpEventDispatcher *event_dispatcher_;

    // Array of multimap (Array of states queues), one for each object
    // key is the timestamp
    std::unique_ptr<std::multimap<unsigned int, std::unique_ptr<ObjectState>> []> state_queue_;

    // Array of state queue locks, this is needed so that the manager thread can do
    // fossil collection and the worker threads can save and restore state when needed.
    std::unique_ptr<std::mutex []> state_queue_lock_;

};

} // warped namespace

#endif
