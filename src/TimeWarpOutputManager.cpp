#include <limits> // for std::numeric_limits<unsigned int>::max()

#include "utility/memory.hpp"
#include "TimeWarpOutputManager.hpp"

namespace warped {

void TimeWarpOutputManager::initialize(unsigned int num_local_objects) {
    output_queue_ = make_unique<std::vector<std::shared_ptr<Event>> []>(num_local_objects);
    output_queue_lock_ = make_unique<std::mutex []>(num_local_objects);
}

void TimeWarpOutputManager::insertEvent(std::shared_ptr<Event> event,
    unsigned int local_object_id) {
    output_queue_lock_[local_object_id].lock();
    output_queue_[local_object_id].push_back(event);
    output_queue_lock_[local_object_id].unlock();
}

unsigned int TimeWarpOutputManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {

    unsigned int retval = std::numeric_limits<unsigned int>::max();
    output_queue_lock_[local_object_id].lock();

    auto min = output_queue_[local_object_id].begin();
    while ((min->get()->timestamp() < gvt) && (min != output_queue_[local_object_id].end())) {
        output_queue_[local_object_id].erase(min);
    }

    if (min != output_queue_[local_object_id].end()) {
        retval = min->get()->timestamp();
    }

    output_queue_lock_[local_object_id].unlock();

    return retval;
}

void TimeWarpOutputManager::fossilCollectAll(unsigned int gvt) {
    for (unsigned int i = 0; i < output_queue_.get()->size(); i++) {
        fossilCollect(gvt, i);
    }
}

std::unique_ptr<std::vector<std::shared_ptr<Event>>>
TimeWarpOutputManager::removeEventsSentAtOrAfter(unsigned int rollback_time,
    unsigned int local_object_id) {

    auto events_to_cancel = make_unique<std::vector<std::shared_ptr<Event>>>();

    output_queue_lock_[local_object_id].lock();

    auto max = std::prev(output_queue_[local_object_id].end());
    while (max->get()->timestamp() >= rollback_time) {
        events_to_cancel->push_back(*max);
        output_queue_[local_object_id].erase(max--);
    }

    output_queue_lock_[local_object_id].unlock();

    return std::move(events_to_cancel);
}

std::size_t TimeWarpOutputManager::size(unsigned int local_object_id) {
    return output_queue_[local_object_id].size();
}

} // namespace warped

