#include <limits>  // for std::numeric_limits<unsigned int>::max();
#include <cstring>  // for memset
#include <cassert> // for assert
#include <iostream>

#include "TimeWarpLocalGVTManager.hpp"
#include "utility/memory.hpp"   // for make_unique

namespace warped {

void TimeWarpLocalGVTManager::initialize(unsigned int num_local_objects) {
    num_local_objects_ = num_local_objects;

    // Prepare local min lvt computation
    local_min_ = make_unique<unsigned int []>(num_local_objects_);
    send_min_ = make_unique<unsigned int []>(num_local_objects_);
    calculated_min_flag_ = make_unique<bool []>(num_local_objects_);

    resetState();
}

unsigned int TimeWarpLocalGVTManager::getMinimumLVT() {
    unsigned int min = std::numeric_limits<unsigned int>::max();
    for (unsigned int i = 0; i < num_local_objects_; i++) {
        min = std::min(min, local_min_[i]);
    }
    resetState();
    return min;
}

bool TimeWarpLocalGVTManager::startGVT() {
    if ((local_gvt_flag_.load() == 0) && !started_local_gvt_) {
        local_gvt_flag_.store(num_local_objects_);
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
    unsigned int local_object_id, unsigned int local_gvt_flag) {

    if (local_gvt_flag > 0 && !calculated_min_flag_[local_object_id]) {
        local_min_[local_object_id] = std::min(send_min_[local_object_id], timestamp);
        calculated_min_flag_[local_object_id] = true;
        local_gvt_flag_.fetch_sub(1);
//        std::cout << "Thread " << thread_id << "time: " << timestamp << std::endl;
    }
}

void TimeWarpLocalGVTManager::sendEventUpdateState(unsigned int timestamp,
    unsigned int local_object_id) {

    if (local_gvt_flag_.load() > 0 && !calculated_min_flag_[local_object_id]) {
        send_min_[local_object_id] = std::min(send_min_[local_object_id], timestamp);
    }
}

void TimeWarpLocalGVTManager::resetState() {
    for (unsigned int i = 0; i < num_local_objects_; i++) {
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        local_min_[i] = std::numeric_limits<unsigned int>::max();
    }
}

unsigned int TimeWarpLocalGVTManager::getLocalGVTFlag() {
    return local_gvt_flag_.load();
}

} // namespace warped


