#ifndef OUTPUT_MANAGER_HPP
#define OUTPUT_MANAGER_HPP

#include <mutex>
#include <vector>

#include "Event.hpp"
#include "TimeWarpEventDispatcher.hpp"

namespace warped {

class OutputManager {
public:
    OutputManager() = default;

    void initialize(unsigned int num_local_objects);

    void insertEvent(std::unique_ptr<Event> event, unsigned int local_object_id);

    unsigned int fossilCollect(unsigned int gvt, unsigned int local_object_id);

    void fossilCollectAll(unsigned int gvt);

    std::size_t size(unsigned int local_object_id);

    // The rollback method will return a vector of negative events that must be sent
    // as anti-messages
    virtual std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        rollback(unsigned int rollback_time, unsigned int local_object_id) = 0;

protected:

    std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        getEventsSentAtOrAfter(unsigned int rollback_time, unsigned int local_object_id);

    // Array of local output queues and locks
    std::unique_ptr<std::vector<std::unique_ptr<Event>> []> output_queue_;
    std::unique_ptr<std::mutex []> output_queue_lock_;
};

}

#endif
