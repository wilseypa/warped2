#ifndef EVENT_SET_HPP
#define EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <mutex>

namespace warped {

class EventSet {
public:
    EventSet() {}

    bool insertEvent ( 
                const std::unique_ptr<Event> event, 
                unsigned int threadId );

    bool handleAntiMessage ( 
                std::unique_ptr<SimulationObject> object, 
                const std::unique_ptr<Event> cancelEvent, 
                unsigned int threadId );

    const std::unique_ptr<Event> getEvent ( 
                std::unique_ptr<SimulationObject> object, 
                unsigned int timeStamp, 
                unsigned int threadId );

    const std::<Event> peekEvent ( 
                std::unique_ptr<SimulationObject> object, 
                unsigned int timeStamp, 
                unsigned int threadId );

    void fossilCollect ( 
                std::unique_ptr<SimulationObject> object, 
                unsigned int timeStamp, 
                unsigned int threadId );

    void fossilCollect ( 
                const std::unique_ptr<Event> event, 
                unsigned int threadId );

    void rollback ( 
                std::unique_ptr<SimulationObject> object, 
                unsigned int rollbackTime, 
                unsigned int threadId );

private:
    std::mutex unprocessedQueueLock;

    std::mutex processedQueueLock;

    std::mutex removedQueueLock;

    //to be modified

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::multiset<const std::unique_ptr<Event>, receiveTimeLessThanEventIdLessThan>*> unprocessedQueue;

    //Queues to hold the processed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> processedQueue;

    //Queues to hold the removed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> removedQueue;

};

} // warped namespace

#endif
