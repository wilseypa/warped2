#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <unordered_map>
#include <mutex>

namespace warped {

class SimulationObject;
class Event;
class STLLTSFQueue;

class TimeWarpEventSet {
public:
    TimeWarpEventSet() = default;

    void initialize ( unsigned int num_of_objects );

    void insertEvent ( 
                unsigned int object_id, 
                const std::unique_ptr<Event> event );

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
    //Lock to protect the unprocessed queue
    std::mutex unprocessed_queue_lock_;

    //Lock to protect the processed queue
    std::mutex processed_queue_lock_;

    //Lock to protect the removed queue
    std::mutex removed_queue_lock_;

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<STLLTSFQueue>> unprocessed_queue_;

    //Queues to hold the processed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> processed_queue_;

    //Queues to hold the removed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> removed_queue_;
};

} // warped namespace

#endif
