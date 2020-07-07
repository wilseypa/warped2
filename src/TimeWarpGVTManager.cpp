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

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::getReportGVTFlagLockShared(){
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::getReportGVTFlagUnlockShared(){
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::getReportGVTFlagLock(){
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::getReportGVTFlagUnlock(){
}

//Dummy Function - Look at the function in TWSynchronousGVTManager
void TimeWarpGVTManager::setReportGVT(bool report_GVT){
    if(report_GVT){
    }
}

/*void TimeWarpGVTManager::progressGVT(int &workers, std::mutex &worker_threads_done_lock_){
    int i = workers;
    worker_threads_done_lock_.lock();
    worker_threads_done_lock_.unlock();
    if (i){
        i =2;
    }
}*/

} // namespace warped
