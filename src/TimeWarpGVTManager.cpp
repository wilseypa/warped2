#include "TimeWarpGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

namespace warped {

void TimeWarpGVTManager::initialize() {
    gvt_state_ = GVTState::IDLE;
    gvt_start = std::chrono::steady_clock::now();
}

void TimeWarpGVTManager::checkProgressGVT() {

    // Get current time
    auto now = std::chrono::steady_clock::now();

    // Calculate time since last GVT start
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_start).count();

    // If token is not already pending and its time to start new calculation
    if ((elapsed >= gvt_period_) && (gvt_state_ == GVTState::IDLE) && (comm_manager_->getID() == 0)) {
        gvt_state_ = GVTState::LOCAL;
        gvt_start = now;
    }

    progressGVT();
}

} // namespace warped
