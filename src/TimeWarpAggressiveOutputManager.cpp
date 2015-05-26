#include "TimeWarpAggressiveOutputManager.hpp"

namespace warped {

// For aggressive cancellation, just simply remove events after straggler event
//  and return them.
std::unique_ptr<std::vector<std::shared_ptr<Event>>>
TimeWarpAggressiveOutputManager::rollback(std::shared_ptr<Event> straggler_event, unsigned int local_object_id) {
    return std::move(removeEventsSentAfter(straggler_event, local_object_id));
}

} // namespace warped

