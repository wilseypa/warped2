#ifndef OUTPUT_MANAGER_HPP
#define OUTPUT_MANAGER_HPP

#include <mutex>
#include <vector>

#include "Event.hpp"
#include "utility/memory.hpp"
#include "TimeWarpEventDispatcher.hpp"

namespace warped {

class OutputManager {
public:
    OutputManager(unsigned int num_objects) :
        output_queue_(make_unique<std::vector<std::unique_ptr<Event>>[]>(num_objects)),
        output_queue_lock_(make_unique<std::mutex[]>(num_objects))
        {}

    void insertEvent(std::unique_ptr<Event> event, unsigned int object_id);

    unsigned int fossilCollect(unsigned int gvt, unsigned int object_id);

    std::size_t size(unsigned int object_id);

    // The rollback method will return a vector of negative events that must be sent
    // as anti-messages
    virtual std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        rollback(unsigned int rollback_time, unsigned int object_id) = 0;

protected:

    std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        getEventsSentAtOrAfter(unsigned int rollback_time, unsigned int object_id);

    // Array of local output queues and locks
    std::unique_ptr<std::vector<std::unique_ptr<Event>> []> output_queue_;
    std::unique_ptr<std::mutex []> output_queue_lock_;
};

}

#endif
