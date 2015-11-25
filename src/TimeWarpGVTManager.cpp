#include "TimeWarpGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

namespace warped {

void TimeWarpGVTManager::initialize() {
    gvt_state_ = GVTState::IDLE;
    gvt_start = std::chrono::steady_clock::now();
}

void TimeWarpGVTManager::checkProgressGVT() {

    if ((gvt_state_ == GVTState::IDLE) && readyToStart()) {
        gvt_state_ = GVTState::LOCAL;
        gvt_start = std::chrono::steady_clock::now();
    }

    progressGVT();
}

} // namespace warped
