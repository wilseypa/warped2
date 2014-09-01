#ifndef TIME_WARP_EVENT_SET_HPP
#define TIME_WARP_EVENT_SET_HPP

/* This class provides the set of APIs needed to handle events in
   an event set.
 */

#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>

namespace warped {

class Event;
class LTSFQueue;

/* Compares two events to see if one has a receive time less than to the other
 */
class compareEvents :
    public std::binary_function<const std::shared_ptr<Event>, 
                                const std::shared_ptr<Event>, 
                                bool > {
public:
    bool operator() ( const std::shared_ptr<Event> first, 
                      const std::shared_ptr<Event> second ) {
        return first->timestamp() < second->timestamp();
    }
};

class TimeWarpEventSet {

public:
    TimeWarpEventSet() = default;

    void initialize (unsigned int num_of_objects, 
                     unsigned int num_of_schedulers,
                     unsigned int num_of_worker_threads);

    void insertEvent (unsigned int obj_id, const std::shared_ptr<Event> event);

    void insertEventVector (unsigned int obj_id, 
                    std::vector<const std::shared_ptr<Event>> events);

    const std::shared_ptr<Event> getEvent (unsigned int thread_id);

    const std::shared_ptr<Event> readLowestEventFromObj (unsigned int obj_id);

    void populateScheduler (unsigned int obj_id, 
                            const std::shared_ptr<Event> old_event);


    //TODO: These APIs might need re-design

    bool handleAntiMessage ( 
                unsigned int object_id, 
                const std::shared_ptr<Event> cancel_event );

    void fossilCollect ( 
                unsigned int object_id, 
                unsigned int timestamp ); 

    void fossilCollectAll(unsigned int timestamp);

    void rollback ( 
                unsigned int object_id, 
                unsigned int rollback_time ); 

private:
    //Number of simulation objects
    unsigned int num_of_objects_;

    //Lock to protect the unprocessed queues
    std::unique_ptr<std::mutex []> unprocessed_queue_lock_;

    //Queues to hold the unprocessed events for each simulation object
    std::vector<std::unique_ptr<std::multiset<const std::shared_ptr<Event>, compareEvents>>> unprocessed_queue_;

    //Lock to protect the processed queues
    std::unique_ptr<std::mutex []> processed_queue_lock_;

    //Queues to hold the processed events for each simulation object
    std::vector<std::unique_ptr<std::vector<const std::shared_ptr<Event>>>> processed_queue_;

    //Number of event schedulers
    unsigned int num_of_schedulers_;

    //Lock to protect the schedule queues
    std::unique_ptr<std::mutex []> schedule_queue_lock_;

    //Queues to hold the scheduled events
    std::vector<std::unique_ptr<LTSFQueue>> schedule_queue_;

    //Map unprocessed queue to a schedule queue
    std::vector<unsigned int> unprocessed_queue_scheduler_map_;

    //Map worker thread to a schedule queue
    std::vector<unsigned int> worker_thread_scheduler_map_;
};

} // warped namespace

#endif
