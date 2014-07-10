#ifndef AGGRESSIVE_OUTPUT_MANAGER_HPP
#define AGGRESSIVE_OUTPUT_MANAGER_HPP

#include "TimeWarpOutputManager.hpp"

namespace warped {

class TimeWarpAggressiveOutputManager : public TimeWarpOutputManager {
public:
    TimeWarpAggressiveOutputManager() = default;

    std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        rollback(unsigned int rollback_time, unsigned int local_object_id);

};

} // namespace warped

#endif
