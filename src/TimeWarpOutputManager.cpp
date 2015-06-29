#include <limits> // for std::numeric_limits<unsigned int>::max()

#include "utility/memory.hpp"
#include "TimeWarpOutputManager.hpp"

namespace warped {

void TimeWarpOutputManager::initialize(unsigned int num_local_objects) {
    output_queue_ = make_unique<std::deque<OutputEvent> []>(num_local_objects);
    num_local_objects_ = num_local_objects;
}

void TimeWarpOutputManager::insertEvent(std::shared_ptr<Event> input_event,
        std::shared_ptr<Event> output_event, unsigned int local_object_id) {
    output_queue_[local_object_id].push_back(OutputEvent(input_event, output_event));
}

unsigned int TimeWarpOutputManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {

    unsigned int retval = std::numeric_limits<unsigned int>::max();

    if (output_queue_[local_object_id].empty()) {
        return retval;
    }

    auto min = output_queue_[local_object_id].begin();
    while ((min != output_queue_[local_object_id].end()) && (min->input_event_->timestamp() < gvt)) {
        output_queue_[local_object_id].pop_front();
        min = output_queue_[local_object_id].begin();
    }

    if (min != output_queue_[local_object_id].end()) {
        retval = min->input_event_->timestamp();
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

    while ((max != output_queue_[local_object_id].rend()) && (*max->input_event_ >= *straggler_event)) {
        auto event = output_queue_[local_object_id].back();
        output_queue_[local_object_id].pop_back();
        // Events are returned in order of LARGEST to SMALLEST
        events_to_cancel->push_back(max->output_event_);
        max = output_queue_[local_object_id].rbegin();
    }

    return std::move(events_to_cancel);
}

std::size_t TimeWarpOutputManager::size(unsigned int local_object_id) {
    return output_queue_[local_object_id].size();
}

} // namespace warped

