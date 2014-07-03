#include "AggressiveOutputManager.hpp"

namespace warped {

std::unique_ptr<std::vector<std::unique_ptr<Event>>>
AggressiveOutputManager::rollback(unsigned int rollback_time, unsigned int local_object_id) {
    return std::move(getEventsSentAtOrAfter(rollback_time, local_object_id));
}

} // namespace warped

