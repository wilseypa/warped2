#include <limits> // for std::numeric_limits<unsigned int>::max()

#include "OutputManager.hpp"

namespace warped {

void OutputManager::insertEvent(std::unique_ptr<Event> event, unsigned int object_id) {

    output_queue_lock_[object_id].lock();
    output_queue_[object_id].push_back(std::move(event));
    output_queue_lock_[object_id].unlock();
}

unsigned int OutputManager::fossilCollect(unsigned int gvt, unsigned int object_id) {

    unsigned int retval = std::numeric_limits<unsigned int>::max();
    output_queue_lock_[object_id].lock();

    auto min = output_queue_[object_id].begin();
    while ((min->get()->timestamp() < gvt) && (min != output_queue_[object_id].end())) {
        output_queue_[object_id].erase(min);
    }

    if (min != output_queue_[object_id].end()) {
        retval = min->get()->timestamp();
    }

    output_queue_lock_[object_id].unlock();

    return retval;
}

std::unique_ptr<std::vector<std::unique_ptr<Event>>>
    OutputManager::getEventsSentAtOrAfter(unsigned int rollback_time, unsigned int object_id) {

    auto events_to_cancel = make_unique<std::vector<std::unique_ptr<Event>>>();

    output_queue_lock_[object_id].lock();

    auto max = std::prev(output_queue_[object_id].end());
    while (max->get()->timestamp() >= rollback_time) {
        events_to_cancel->push_back(std::move(*max));
        output_queue_[object_id].erase(max--);
    }

    output_queue_lock_[object_id].unlock();

    return std::move(events_to_cancel);
}

std::size_t OutputManager::size(unsigned int object_id) {
    return output_queue_[object_id].size();
}

} // namespace warped

