#include <algorithm>    // for std::min()
#include <cassert>

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

    TimeWarpGVTManager::initialize();
}

bool TimeWarpSynchronousGVTManager::readyToStart() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_stop).count();
    return  (elapsed >= gvt_period_);
}

void TimeWarpSynchronousGVTManager::progressGVT() {
    if (gvt_state_ == GVTState::LOCAL) {

        local_gvt_flag_.store(1);
        pthread_barrier_wait(&min_report_barrier_);

        gvt_state_ = GVTState::GLOBAL;

        int64_t total_msg_count;
        while (true) {
            comm_manager_->flushMessages();
            comm_manager_->handleMessages();
            int64_t local_msg_count = msg_count_.load();
            comm_manager_->sumAllReduceInt64(&local_msg_count, &total_msg_count);
            if(total_msg_count == 0)
                break;
        }

        unsigned int local_min = std::numeric_limits<unsigned int>::max();
        for (unsigned int i = 0; i <= num_worker_threads_; i++) {
            local_min = std::min(local_min, std::min(local_min_[i], send_min_[i]));
            local_min_[i] = std::numeric_limits<unsigned int>::max();
            send_min_[i] = std::numeric_limits<unsigned int>::max();
        }

        comm_manager_->minAllReduceUint(&local_min, &gVT_);

        gvt_updated_ = true;

        local_gvt_flag_.store(0);
        pthread_barrier_wait(&min_report_barrier_);

        gvt_stop = std::chrono::steady_clock::now();
        gvt_state_ = GVTState::IDLE;
    }
}

Color TimeWarpSynchronousGVTManager::sendEventUpdate(std::shared_ptr<Event>& event) {
    unused(event);
    msg_count_++;

    return Color::WHITE;
}

void TimeWarpSynchronousGVTManager::receiveEventUpdate(std::shared_ptr<Event>& event, Color color) {
    unused(event);
    unused(color);

    msg_count_--;
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

void TimeWarpSynchronousGVTManager::reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) {
    if (local_gvt_flag_.load() > 0) {
        send_min_[thread_id] = std::min(timestamp, send_min_[thread_id]);
    }
}

unsigned int TimeWarpSynchronousGVTManager::getLocalGVTFlag() {
    return local_gvt_flag_.load();
}

} // namespace warped
