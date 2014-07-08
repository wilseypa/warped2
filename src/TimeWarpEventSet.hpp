#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <mutex>

namespace warped {

class SimulationObject;
class Event;

class TimeWarpEventSet {
public:
    TimeWarpEventSet();

    bool insertEvent ( 
                const std::unique_ptr<Event> event, 
                unsigned int thread_id );

    bool handleAntiMessage ( 
                SimulationObject* object, 
                const std::unique_ptr<Event> cancel_event, 
                unsigned int thread_id );

    const std::unique_ptr<Event> getEvent ( 
                SimulationObject* object, 
                unsigned int timestamp, 
                unsigned int thread_id );

    const std::<Event> peekEvent ( 
                SimulationObject* object, 
                unsigned int timestamp, 
                unsigned int thread_id );

    void fossilCollect ( 
                SimulationObject* object, 
                unsigned int timestamp, 
                unsigned int thread_id );

    void fossilCollect ( 
                const std::unique_ptr<Event> event, 
                unsigned int thread_id );

    void rollback ( 
                SimulationObject* object, 
                unsigned int rollback_time, 
                unsigned int thread_id );

private:
    std::mutex unprocessed_queue_mutex_;

    std::mutex processed_queue_mutex_;

    std::mutex removed_queue_mutex_;

    //to be modified

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::multiset<const std::unique_ptr<Event>, receiveTimeLessThanEventIdLessThan>*> unprocessed_queue_;

    //Queues to hold the processed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> processed_queue_;

    //Queues to hold the removed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> removed_queue_;

};

} // warped namespace

#endif
