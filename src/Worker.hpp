/*
----notes----


Class Worker() {
    worker.gvtContrib <- 0
    worker.outMessage <- NULL

    function rollback(*e) (rollback to event e)
    function processEvent(*e) (process event from LTSF queue -> check LP -> antimessage -> rollback -> exec)

    worker **thread**
}
*/

#ifndef WORKER_HPP
#define WORKER_HPP

#include "HouseKeeping.hpp"

namespace warped {
    //are these needed?
    //enum class GVTState { IDLE, LOCAL, GLOBAL };
    //enum class Color { WHITE, RED };

class Worker {
public:
    Worker(std::unique_ptr<TimeWarpTerminationManager> termination_manager);

    virtual void initialize();
    virtual ~Worker() = default;

    virtual unsigned int gvtContrib() = 0;

    virtual void outMessage() = NULL;

    virtual void coastForward(std::shared_ptr<Event> straggler_event, 
                                std::shared_ptr<Event> restored_state_event);

    virtual void rollback(std::shared_ptr<Event> straggler_event);
    
    virtual void processEvent(unsigned int id);

    virtual void thread();

private:
    const std::unique_ptr<TimeWarpTerminationManager> termination_manager_; // jack made this private, is this the right spot?

protected:
    std::shared_ptr<Event> event;
};

}

#endif