#ifndef OUTPUT_MANAGER_HPP
#define OUTPUT_MANAGER_HPP

#include <vector>
#include <deque>

#include "Event.hpp"
#include "TimeWarpEventDispatcher.hpp"

/* This class contains output queues for each local object and serves as a base class for
 * a specific cancellation technique.
 */

namespace warped {

class TimeWarpOutputManager {
public:
    TimeWarpOutputManager() = default;

    // Creates an output queue for each object as well as locks for each output queue.
    void initialize(unsigned int num_local_objects);

    // Insert an event into the output queue for the specified object
    void insertEvent(std::shared_ptr<Event> event, unsigned int local_object_id);

    // Remove any events from the output queue before the gvt for the specified object
    unsigned int fossilCollect(unsigned int gvt, unsigned int local_object_id);

    // Number of events in the output queue for the specified object.
    std::size_t size(unsigned int local_object_id);

    // The rollback method will return a vector of negative events that must be sent
    // as anti-messages
    virtual std::unique_ptr<std::vector<std::shared_ptr<Event>>>
        rollback(std::shared_ptr<Event> straggler_event, unsigned int local_object_id) = 0;

protected:

    std::unique_ptr<std::vector<std::shared_ptr<Event>>>
        removeEventsSentAfter(std::shared_ptr<Event> straggler_event, unsigned int local_object_id);

    // Array of local output queues and locks
    std::unique_ptr<std::deque<std::shared_ptr<Event>> []> output_queue_;

    unsigned int num_local_objects_;
};

}

#endif
