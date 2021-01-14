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

//would the namespace still be warped?
namespace warped {
    //are these needed?
    //enum class GVTState { IDLE, LOCAL, GLOBAL };
    //enum class Color { WHITE, RED };

class Worker {
public:
    Worker();

    virtual void initialize();
    virtual ~Worker() = default;

    // in the doc these are worker.value
    // are these values being held in a class somewhere else?
    virtual unsigned int gvtContrib() = 0;

    virtual void outMessage(); // NULL

    virtual void rollback(int e);
    
    virtual void processEvent(int e);



protected:
    int e = 0; //dummy code, this will be an event e
};

}

#endif