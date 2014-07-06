#ifndef AGGRESSIVE_OUTPUT_MANAGER_HPP
#define AGGRESSIVE_OUTPUT_MANAGER_HPP

#include "OutputManager.hpp"

namespace warped {

class AggressiveOutputManager : public OutputManager {
public:
    AggressiveOutputManager() = default;

    std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        rollback(unsigned int rollback_time, unsigned int local_object_id);

};

} // namespace warped

#endif
