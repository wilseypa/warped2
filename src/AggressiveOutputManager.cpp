#include "AggressiveOutputManager.hpp"

namespace warped {

std::unique_ptr<std::vector<std::unique_ptr<Event>>>
AggressiveOutputManager::rollback(unsigned int rollback_time, unsigned int object_id) {
    return std::move(getEventsSentAtOrAfter(rollback_time, object_id));
}

} // namespace warped

