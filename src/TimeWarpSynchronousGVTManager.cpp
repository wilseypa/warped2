#include <algorithm>    // for std::min()
#include <cassert>

#include "TimeWarpSynchronousGVTManager.hpp"
#include "utility/warnings.hpp"
#include "utility/memory.hpp"           // for make_unique

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTSynchTrigger)

namespace warped {

enum class Color;

void TimeWarpSynchronousGVTManager::initialize() {
    pthread_barrier_init(&min_report_barrier_, NULL, num_worker_threads_+1);

    local_min_ = make_unique<unsigned int []>(num_worker_threads_+1);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_+1);

    for (unsigned int i = 0; i <= num_worker_threads_; i++) {
        local_min_[i] = std::numeric_limits<unsigned int>::max();
        send_min_[i] = std::numeric_limits<unsigned int>::max();
    }

    recv_min_ = std::numeric_limits<unsigned int>::max();

    report_gvt_ = false;

    WARPED_REGISTER_MSG_HANDLER(TimeWarpSynchronousGVTManager, receiveGVTSynchTrigger, GVTSynchTrigger)

    TimeWarpGVTManager::initialize();
}

void TimeWarpSynchronousGVTManager::receiveGVTSynchTrigger(std::unique_ptr<TimeWarpKernelMessage> kmsg){
    auto msg = unique_cast<TimeWarpKernelMessage, GVTSynchTrigger>(std::move(kmsg));
std::cout << "RECEIVE GVT TOKEN NODE = " << comm_manager_->getID() << std::endl; 
    report_gvt_lock_.lock();
    report_gvt_ = true;
    report_gvt_lock_.unlock();
}

void TimeWarpSynchronousGVTManager::triggerSynchGVTCalculation(){
    // Start a GVT calculation cycle, only node 0 can trigger a GVT calculation
    for (unsigned int i = 1; i < comm_manager_->getNumProcesses(); i++) {
        auto gvt_trigger_msg = make_unique<GVTSynchTrigger>(0, i);
        comm_manager_->insertMessage(std::move(gvt_trigger_msg));
    }
}

bool TimeWarpSynchronousGVTManager::readyToStart() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_stop).count();
    return  (elapsed >= gvt_period_);
}

void TimeWarpSynchronousGVTManager::progressGVT(unsigned int &local_gvt_passed_in) {
    
    //report_gvt_lock_.lock();
    //report_gvt_ = true;
    //report_gvt_lock_.unlock();

    //pthread_barrier_wait(&min_report_barrier_);

// if (worker_threads_dumped) {}
//std::cout << "====== IN GVT 0 NODE = " << comm_manager_->getID() << std::endl;
    report_gvt_lock_.lock();
    report_gvt_ = false;
    report_gvt_lock_.unlock();
//std::cout << "====== IN GVT 1 NODE = " << comm_manager_->getID() << std::endl;
    pthread_barrier_wait(&min_report_barrier_);

//std::cout << "====== IN GVT 2 NODE = " << comm_manager_->getID() << std::endl;
    // Collect GVT from all of the worker threads 
    unsigned int local_min = recv_min_;
//std::cout << "====== IN Recieve Min = " << recv_min_ << " NODE = " << comm_manager_->getID() << std::endl;
    recv_min_ = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i <= num_worker_threads_; i++) {
        local_min = std::min(local_min, local_min_[i]);
        local_min_[i] = std::numeric_limits<unsigned int>::max();
    }
    
//std::cout << "====== IN GVT 3 Local Min = " << local_min << " NODE = " << comm_manager_->getID() << std::endl;
    
    local_gvt_passed_in = local_min;

    gvt_stop = std::chrono::steady_clock::now();
    //access_gvt_lock_.lock();
    //comm_manager_->minAllReduceUint(&local_min, &next_gvt_);
    //access_gvt_lock_.unlock();

     
// worker_threads_dumped = false
// }
}

Color TimeWarpSynchronousGVTManager::sendEventUpdate(std::shared_ptr<Event>& event) {
    unused(event);

    Color color = color_.load();

    if (color == Color::WHITE) {
        white_msg_count_++;
    }

    return color;
}

void TimeWarpSynchronousGVTManager::receiveEventUpdate(std::shared_ptr<Event>& event, Color color) {

    if (color == Color::WHITE) {
        white_msg_count_--;
    }// else {
        recv_min_ = std::min(recv_min_, event->timestamp());
//    }
}

bool TimeWarpSynchronousGVTManager::gvtUpdated() {
    if (gvt_updated_) {
        gvt_updated_ = false;
        return true;
    }
    return false;
}

void TimeWarpSynchronousGVTManager::reportThreadMin(unsigned int timestamp, unsigned int thread_id,
                                                    unsigned int local_gvt_flag) {
    if (local_gvt_flag > 0) {
        local_min_[thread_id] = timestamp;

        pthread_barrier_wait(&min_report_barrier_);
        pthread_barrier_wait(&min_report_barrier_);
    }
}

void TimeWarpSynchronousGVTManager::reportThreadMin(unsigned int timestamp, unsigned int thread_id) {
        local_min_[thread_id] = timestamp;   
}

void TimeWarpSynchronousGVTManager::reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) {
    if (local_gvt_flag_.load() > 0) {
        send_min_[thread_id] = std::min(timestamp, send_min_[thread_id]);
    }
}

unsigned int TimeWarpSynchronousGVTManager::getLocalGVTFlag() {
    return 10;
}

bool TimeWarpSynchronousGVTManager::getGVTFlag() {
    bool report_gvt_status;

    report_gvt_lock_.lock_shared();
    report_gvt_status = report_gvt_;
    report_gvt_lock_.unlock_shared();

    return report_gvt_status;
}

void TimeWarpSynchronousGVTManager::workerThreadGVTBarrierSync(){
    pthread_barrier_wait(&min_report_barrier_);
}

void TimeWarpSynchronousGVTManager::getReportGVTFlagLockShared(){ 
    report_gvt_lock_.lock_shared(); 
}
    
void TimeWarpSynchronousGVTManager::getReportGVTFlagUnlockShared(){ 
    report_gvt_lock_.unlock_shared(); 
}

void TimeWarpSynchronousGVTManager::getReportGVTFlagLock(){ 
    report_gvt_lock_.lock(); 
}
    
void TimeWarpSynchronousGVTManager::getReportGVTFlagUnlock(){ 
    report_gvt_lock_.unlock(); 
}

void TimeWarpSynchronousGVTManager::setReportGVT(bool report_GVT){ 
    report_gvt_ = report_GVT;
}

unsigned int TimeWarpSynchronousGVTManager::getNextGVT(){ 
    return next_gvt_;
}

void TimeWarpSynchronousGVTManager::setNextGVT(unsigned int new_GVT){ 
    gVT_ = next_gvt_;
    next_gvt_ = new_GVT;
    gvt_updated_ = true;
}
} // namespace warped
