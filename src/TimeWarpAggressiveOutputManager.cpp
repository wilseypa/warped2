#include "TimeWarpAggressiveOutputManager.hpp"

namespace warped {

std::unique_ptr<std::vector<std::shared_ptr<Event>>>
TimeWarpAggressiveOutputManager::rollback(std::shared_ptr<Event> straggler_event, unsigned int local_object_id) {
    return std::move(removeEventsSentAfter(straggler_event, local_object_id));
}

} // namespace warped

