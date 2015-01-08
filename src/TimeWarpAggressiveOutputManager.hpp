#ifndef AGGRESSIVE_OUTPUT_MANAGER_HPP
#define AGGRESSIVE_OUTPUT_MANAGER_HPP

#include "TimeWarpOutputManager.hpp"

/* This class is a subclass of TimeWarpOutputManager that implements a rollback method for
 * aggressive cancellation. The rollback method returns a vector of events that must be cancelled
 * and sent as anti-messages.
 */

namespace warped {

class TimeWarpAggressiveOutputManager : public TimeWarpOutputManager {
public:
    TimeWarpAggressiveOutputManager() = default;

    std::unique_ptr<std::vector<std::shared_ptr<Event>>>
        rollback(std::shared_ptr<Event> straggler_event, unsigned int local_object_id);

};

} // namespace warped

#endif
