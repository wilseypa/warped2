#include "StateManager.hpp"

namespace warped {

unsigned int StateManager::restoreState(unsigned int rollback_time, unsigned int object_id) {
    rollback_time = 0;
    object_id = 0;
    return rollback_time;
}

void StateManager::fossilCollect(unsigned int gvt, unsigned int object_id) {
    gvt = 0;
    object_id = 0;
}

} // namespace warped
