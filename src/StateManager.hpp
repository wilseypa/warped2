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

    StateManager(unsigned int num_objects) :
        state_queue_(make_unique<std::multimap<unsigned int, std::unique_ptr<ObjectState>> []>
            (num_objects)),
        state_queue_lock_(make_unique<std::mutex []>(num_objects))
        {}

    unsigned int fossilCollect(unsigned int gvt, unsigned int local_object_id);

    void fossilCollectAll(unsigned int gvt);

    unsigned int restoreState(unsigned int rollback_time, unsigned int local_object_id,
        SimulationObject *object);

    std::size_t size(unsigned int local_object_id);

    virtual void saveState(unsigned int current_time, unsigned int local_object_id,
        SimulationObject *object) = 0;

protected:

    // Array of multimap (Array of states queues), one for each object
    // key is the timestamp
    std::unique_ptr<std::multimap<unsigned int, std::unique_ptr<ObjectState>> []> state_queue_;

    // Array of state queue locks, this is needed so that the manager thread can do
    // fossil collection and the worker threads can save and restore state when needed.
    std::unique_ptr<std::mutex []> state_queue_lock_;

};

} // warped namespace

#endif
