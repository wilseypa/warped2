#include <limits>  // for std::numeric_limits<unsigned int>::max();
#include <cstring>  // for memset
#include <cassert> // for assert
#include <iostream>

#include "TimeWarpLocalGVTManager.hpp"
#include "utility/memory.hpp"   // for make_unique

namespace warped {

void TimeWarpLocalGVTManager::initialize(unsigned int num_worker_threads) {
    num_worker_threads_ = num_worker_threads;

    // Prepare local min lvt computation
    local_min_ = make_unique<unsigned int []>(num_worker_threads_+1);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_+1);
    calculated_min_flag_ = make_unique<bool []>(num_worker_threads_+1);

    resetState();
}

unsigned int TimeWarpLocalGVTManager::getMinimumLVT() {
    unsigned int min = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i <= num_worker_threads_; i++) {
        min = std::min(min, local_min_[i]);
    }
    return min;
}

bool TimeWarpLocalGVTManager::startGVT() {
    if ((local_gvt_flag_.load() == 0) && !started_local_gvt_) {
        resetState();
        local_gvt_flag_.store(num_worker_threads_);
        started_local_gvt_ = true;
        return true;
    }
    return false;
}

bool TimeWarpLocalGVTManager::completeGVT() {
    if ((local_gvt_flag_.load() == 0) && started_local_gvt_) {
        unsigned int local_min = getMinimumLVT();
        gVT_ = local_min;
        started_local_gvt_ = false;
        return true;
    }
    return false;
}

void TimeWarpLocalGVTManager::receiveEventUpdateState(unsigned int timestamp,
    unsigned int thread_id, unsigned int local_gvt_flag) {

    // If this thread has not yet reported a minimum, then report it.
    if (local_gvt_flag > 0 && !calculated_min_flag_[thread_id]) {
        local_min_[thread_id] = std::min(send_min_[thread_id], timestamp);
        calculated_min_flag_[thread_id] = true;
        local_gvt_flag_.fetch_sub(1);
    }
}

void TimeWarpLocalGVTManager::sendEventUpdateState(unsigned int timestamp,
    unsigned int thread_id) {

    // If this thread has not yet reported a minimum, make sure to report
    //  any sends to avoid the "simultaneous reporting problem"
    if (local_gvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        send_min_[thread_id] = std::min(send_min_[thread_id], timestamp);
    }
}

void TimeWarpLocalGVTManager::resetState() {
    for (unsigned int i = 0; i < num_worker_threads_+1; i++) {
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        local_min_[i] = std::numeric_limits<unsigned int>::max();
    }

    // Manager thread will not receive any events
    calculated_min_flag_[num_worker_threads_] = true;
}

unsigned int TimeWarpLocalGVTManager::getLocalGVTFlag() {
    return local_gvt_flag_.load();
}

} // namespace warped


