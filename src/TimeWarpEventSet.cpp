#include "TimeWarpEventSet.hpp"
#include "STLLTSFQueue.hpp"

namespace warped {

void TimeWarpEventSet::initialize( unsigned int num_of_objects, 
                                   unsigned int num_of_schedulers ) {

    num_of_objects_ = num_of_objects;
    num_of_schedulers_ = num_of_schedulers;

    //Create the unprocessed and processed queues and their locks
    for (unsigned int obj_index = 0; obj_index < num_of_objects; obj_index++) {
        unprocessed_queue_.push_back(
                new std::multiset<const std::shared_ptr<Event>, compareEvents>);
        processed_queue_.push_back(
                new std::multiset<const std::shared_ptr<Event>, compareEvents>);
    }
    unprocessed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);
    processed_queue_lock_ = make_unique<std::mutex []>(num_of_objects);

    //Create the event schedulers and their locks
    for (unsigned int scheduler_index = 0; 
            scheduler_index < num_of_schedulers; scheduler_index++) {
        event_scheduler_.push_back(new STLLTSFQueue());
    }
    schedule_queue_lock_ = make_unique<std::mutex []>(num_of_schedulers);
}

void TimeWarpEventSet::insertEvent( unsigned int obj_id, const std::shared_ptr<Event> event ) {

    unprocessed_queue_lock_.lock();
    unprocessed_queue[obj_id]->push(event);
    unprocessed_queue_lock_.unlock();

    //TODO: add the scheduler part without using lookup
}

void insertEventVector( unsigned int object_id, 
                        std::vector<const std::shared_ptr<Event>> event_vector) {

}

const std::shared_ptr<Event> TimeWarpEventSet::getEvent () { 

}

const std::shared_ptr<Event> readLowestEventFromObj(unsigned int object_id) {

}

//TODO: APIs not ready
bool TimeWarpEventSet::handleAntiMessage ( 
                                    unsigned int obj_id, 
                                    const std::unique_ptr<Event> cancel_event ) { 

}

void TimeWarpEventSet::fossilCollect ( 
                                    SimulationObject* object, 
                                    unsigned int timestamp, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::fossilCollect ( 
                                    const std::unique_ptr<Event> event, 
                                    unsigned int thread_id ) {

}

void TimeWarpEventSet::rollback (   SimulationObject* object, 
                                    unsigned int rollback_time, 
                                    unsigned int thread_id ) {

}

} // namespace warped

