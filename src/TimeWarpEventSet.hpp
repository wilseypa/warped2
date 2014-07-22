#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <unordered_map>
#include <mutex>

namespace warped {

class Event;
class STLLTSFQueue;
class TimeWarpEventScheduler;

class TimeWarpEventSet {
public:
    TimeWarpEventSet = default;

    void initialize ( 
                unsigned int num_of_objects, 
                unsigned int num_of_schedulers );

    void insertEvent ( 
                unsigned int object_id, 
                const std::unique_ptr<Event> event );

    bool handleAntiMessage ( 
                unsigned int object_id, 
                const std::unique_ptr<Event> cancel_event );

    const std::unique_ptr<Event> getEvent ( 
                unsigned int object_id, 
                unsigned int timestamp );

    const std::<Event> peekEvent ( 
                unsigned int object_id, 
                unsigned int timestamp );

    void fossilCollect ( 
                unsigned int object_id, 
                unsigned int timestamp ); 

    void fossilCollect ( const std::unique_ptr<Event> event );

    void rollback ( 
                unsigned int object_id, 
                unsigned int rollback_time ); 

private:
    //Lock to protect the unprocessed queue
    std::mutex unprocessed_queue_lock_;

    //Lock to protect the processed queue
    std::mutex processed_queue_lock_;

    //Number of simulation objects
    unsigned int num_of_objects_;

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<STLLTSFQueue>> unprocessed_queue_;

    //Queues to hold the processed events for each simulation object
    std::vector<std::vector<const std::unique_ptr<Event>>*> processed_queue_;

    //Number of event schedulers
    unsigned int num_of_schedulers_;

    //Event schedulers
    std::vector<std::unique_ptr<TimeWarpEventScheduler>> event_scheduler_;
};

} // warped namespace

#endif
