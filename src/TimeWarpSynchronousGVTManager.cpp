#include <algorithm>    // for std::min()
#include <cassert>
#include <mutex>

#include "TimeWarpSynchronousGVTManager.hpp"
#include "utility/warnings.hpp"
#include "utility/memory.hpp"           // for make_unique

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

    TimeWarpGVTManager::initialize();
}

bool TimeWarpSynchronousGVTManager::readyToStart() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_stop).count();
    return  (elapsed >= gvt_period_);
}

void TimeWarpSynchronousGVTManager::progressGVT(unsigned int &local_gvt_passed_in) {
    
    setGVTEstCycle(false);
    workerThreadGVTBarrierSync();

    // Collect GVT from all of the worker threads 
    unsigned int local_min = recv_min_;

    recv_min_ = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i <= num_worker_threads_; i++) {
        local_min = std::min(local_min, local_min_[i]);
        local_min_[i] = std::numeric_limits<unsigned int>::max();
    }

    local_gvt_passed_in = local_min;

    gvt_stop = std::chrono::steady_clock::now();
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
    } else {
        recv_min_ = std::min(recv_min_, event->timestamp());
    }
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

        workerThreadGVTBarrierSync();
        workerThreadGVTBarrierSync();
    }
}

void TimeWarpSynchronousGVTManager::reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) {
    if (local_gvt_flag_.load() > 0) {
        send_min_[thread_id] = std::min(timestamp, send_min_[thread_id]);
    }
}

unsigned int TimeWarpSynchronousGVTManager::getLocalGVTFlag() {
    return local_gvt_flag_.load();
}

void TimeWarpSynchronousGVTManager::setGVTEstCycle(bool status) {
    gvt_est_cycle_lock_.lock();
    gvt_est_cycle_ = status;
    gvt_est_cycle_lock_.unlock();
}

void TimeWarpSynchronousGVTManager::workerThreadGVTBarrierSync() {
    pthread_barrier_wait(&min_report_barrier_);
}

unsigned int TimeWarpSynchronousGVTManager::getPrevGVT() {
    return prev_gvt_;
}

void TimeWarpSynchronousGVTManager::setPrevGVT(unsigned int prev_gvt) {
    prev_gvt_lock_.lock();
    prev_gvt_ = prev_gvt;
    prev_gvt_lock_.unlock();
}

void TimeWarpSynchronousGVTManager::setGVT(unsigned int new_gvt) {
    gvt_lock_.lock();
    gVT_ = new_gvt;
    gvt_lock_.unlock();
}

} // namespace warped
