#include <limits> // for std::numeric_limits<unsigned int>::max()

#include "utility/memory.hpp"
#include "TimeWarpOutputManager.hpp"

namespace warped {

void TimeWarpOutputManager::initialize(unsigned int num_local_objects) {
    output_queue_ = make_unique<std::vector<std::shared_ptr<Event>> []>(num_local_objects);
    num_local_objects_ = num_local_objects;
}

void TimeWarpOutputManager::insertEvent(std::shared_ptr<Event> event,
    unsigned int local_object_id) {
    output_queue_[local_object_id].push_back(event);
}

unsigned int TimeWarpOutputManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {

    unsigned int retval = std::numeric_limits<unsigned int>::max();

    if (output_queue_[local_object_id].empty()) {
        return retval;
    }

    auto min = output_queue_[local_object_id].begin();
    while ((min != output_queue_[local_object_id].end()) && (min->get()->timestamp() < gvt)) {
        min = output_queue_[local_object_id].erase(min);
    }

    if (min != output_queue_[local_object_id].end()) {
        retval = min->get()->timestamp();
    }

    return retval;
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>>
TimeWarpOutputManager::removeEventsSentAfter(std::shared_ptr<Event> straggler_event,
    unsigned int local_object_id) {

    // We need to get all the events that are STRICTLY GREATER than the straggler event.

    // Empty vector of events
    auto events_to_cancel = make_unique<std::vector<std::shared_ptr<Event>>>();

    auto max = output_queue_[local_object_id].rbegin(); // Start at the largest event

    while (max != output_queue_[local_object_id].rend()) {
        if (**max <= *straggler_event) {
            // Output queues are assumed to be in order, so we are safe to stop when we reach
            // an event that is less than or equal to the straggler.
            break;
        } else {
            // Events are in order of LARGEST to SMALLEST
            max++;
            auto event = output_queue_[local_object_id].back();
            output_queue_[local_object_id].pop_back();
            events_to_cancel->push_back(event);
        }
    }

    return std::move(events_to_cancel);
}

std::size_t TimeWarpOutputManager::size(unsigned int local_object_id) {
    return output_queue_[local_object_id].size();
}

} // namespace warped

