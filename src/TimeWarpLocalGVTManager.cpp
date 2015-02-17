#include <limits>  // for std::numeric_limits<unsigned int>::max();
#include <cstring>  // for memset
#include <cassert> // for assert
#include <iostream>

#include "TimeWarpLocalGVTManager.hpp"
#include "utility/memory.hpp"   // for make_unique

namespace warped {

void TimeWarpLocalGVTManager::initialize(unsigned int num_worker_threads,
                                         unsigned int num_local_objects) {
    num_worker_threads_ = num_worker_threads;
    num_local_objects_ = num_local_objects;

    continuous_straggler_flags_ = make_unique<unsigned int []>(num_local_objects_);
    std::memset(continuous_straggler_flags_.get(), 0, num_local_objects_*sizeof(unsigned int));

    local_gvt_straggler_flags_ = make_unique<unsigned int []>(num_local_objects_);
    std::memset(local_gvt_straggler_flags_.get(), 0, num_local_objects_*sizeof(unsigned int));

    // Prepare local min lvt computation
    min_lvt_ = make_unique<unsigned int []>(num_worker_threads_);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_);
    calculated_min_flag_ = make_unique<bool []>(num_worker_threads_);

    resetState();
}

unsigned int TimeWarpLocalGVTManager::getMinimumLVT() {
    unsigned int min = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        min = std::min(min, min_lvt_[i]);
    }
    resetState();
    return min;
}

bool TimeWarpLocalGVTManager::startGVT() {
    if ((min_lvt_flag_.load() == 0) && !started_local_gvt_) {
        straggler_flags_lock_.lock();

        min_lvt_flag_.store(num_worker_threads_);
        started_local_gvt_ = true;

        std::memcpy(local_gvt_straggler_flags_.get(), continuous_straggler_flags_.get(),
            num_local_objects_*sizeof(unsigned int));
        local_gvt_straggler_count_ = continuous_straggler_count_;

        straggler_flags_lock_.unlock();

        return true;
    }
    return false;
}

bool TimeWarpLocalGVTManager::completeGVT() {
    if ((min_lvt_flag_.load() == 0) && started_local_gvt_  && (local_gvt_straggler_count_ == 0)) {
        unsigned int local_min_lvt = getMinimumLVT();
        gVT_ = local_min_lvt;
        started_local_gvt_ = false;
        return true;
    }
    return false;
}

void TimeWarpLocalGVTManager::receiveEventUpdateState(unsigned int timestamp, unsigned int thread_id) {
    if (min_lvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        min_lvt_[thread_id] = std::min(send_min_[thread_id], timestamp);
        calculated_min_flag_[thread_id] = true;
        min_lvt_flag_.fetch_sub(1);
//        std::cout << "Thread " << thread_id << "time: " << timestamp << std::endl;
    }
}

void TimeWarpLocalGVTManager::sendEventUpdateState(unsigned int timestamp, unsigned int thread_id) {
    if (min_lvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        send_min_[thread_id] = std::min(send_min_[thread_id], timestamp);
    }
}

void TimeWarpLocalGVTManager::resetState() {
    for (unsigned int i = 0; i < num_worker_threads_; i++) {
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        min_lvt_[i] = std::numeric_limits<unsigned int>::max();
    }
}

void TimeWarpLocalGVTManager::reportStraggler(unsigned int local_object_id) {
    straggler_flags_lock_.lock();

    continuous_straggler_flags_[local_object_id]++;
    continuous_straggler_count_++;

    assert(continuous_straggler_flags_[local_object_id] <= 2);

    straggler_flags_lock_.unlock();
}

void TimeWarpLocalGVTManager::reportRollback(unsigned int local_object_id) {
    straggler_flags_lock_.lock();

    assert(continuous_straggler_flags_[local_object_id] != 0);
    assert(continuous_straggler_count_ != 0);

    if (continuous_straggler_flags_[local_object_id] > 0) {
        continuous_straggler_flags_[local_object_id]--;
        continuous_straggler_count_--;

        if (started_local_gvt_) {
            assert(local_gvt_straggler_flags_[local_object_id] != 0);
            assert(local_gvt_straggler_count_ != 0);
            local_gvt_straggler_flags_[local_object_id]--;
            local_gvt_straggler_count_--;
        }
    }

    straggler_flags_lock_.unlock();
}

} // namespace warped


