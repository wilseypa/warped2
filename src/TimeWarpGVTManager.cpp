#include "TimeWarpGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

namespace warped {

void TimeWarpGVTManager::initialize() {
    gvt_state_ = GVTState::IDLE;
    gvt_start = std::chrono::steady_clock::now();
    gvt_stop = std::chrono::steady_clock::now();
}

void TimeWarpGVTManager::checkProgressGVT() {

    if ((gvt_state_ == GVTState::IDLE) && readyToStart()) {
        gvt_state_ = GVTState::LOCAL;
        gvt_start = std::chrono::steady_clock::now();
    }

    progressGVT();
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
bool TimeWarpGVTManager::getGVTFlag(){
    return false;
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::workerThreadGVTBarrierSync(){
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::reportThreadMin(unsigned int timestamp, unsigned int thread_id){
    timestamp++; thread_id++;
}

} // namespace warped
