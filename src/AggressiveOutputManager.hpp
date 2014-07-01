#ifndef AGGRESSIVE_OUTPUT_MANAGER_HPP
#define AGGRESSIVE_OUTPUT_MANAGER_HPP

#include "OutputManager.hpp"

namespace warped {

class AggressiveOutputManager : public OutputManager {
public:
    AggressiveOutputManager(unsigned int num_objects) :
        OutputManager(num_objects) {}

    std::unique_ptr<std::vector<std::unique_ptr<Event>>>
        rollback(unsigned int rollback_time, unsigned int object_id);

};

} // namespace warped

#endif
