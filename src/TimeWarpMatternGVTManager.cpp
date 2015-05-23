#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()
#include <cassert>

#include "TimeWarpMatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTUpdateMessage)

namespace warped {

void TimeWarpMatternGVTManager::initialize() {
    WARPED_REGISTER_MSG_HANDLER(TimeWarpMatternGVTManager, receiveMatternGVTToken, MatternGVTToken);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpMatternGVTManager, receiveGVTUpdate, GVTUpdateMessage);
    gvt_start = std::chrono::steady_clock::now();
}

unsigned int TimeWarpMatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void TimeWarpMatternGVTManager::receiveEventUpdateState(MatternColor color) {
    state_.lock_.lock();
    if (color == MatternColor::WHITE) {
        state_.white_msg_counter_--;
    }
    state_.lock_.unlock();
}

MatternColor TimeWarpMatternGVTManager::sendEventUpdateState(unsigned int timestamp) {
    state_.lock_.lock();

    if (state_.color_ == MatternColor::WHITE) {
        state_.white_msg_counter_++;
    } else {
        min_red_msg_timestamp_ = std::min(min_red_msg_timestamp_, timestamp);
    }

    MatternColor color = state_.color_;

    state_.lock_.unlock();

    return color;
}

bool TimeWarpMatternGVTManager::startGVT() {

    // Only node 0 can start a new GVT calculation
    // NOTE: This will work for a single node simulation also
    if (comm_manager_->getID() != 0 || !master_can_start_) {
        return false;
    }

    // Get current time
    auto now = std::chrono::steady_clock::now();

    // Calculate time since last GVT start
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_start).count();

    // If token is not already pending and its time to start new calculation
    if (gVT_token_pending_ == false && elapsed >= gvt_period_) {

        if (comm_manager_->getNumProcesses() > 1) {
            state_.lock_.lock();
            assert(state_.color_ == MatternColor::WHITE);
            state_.color_ = MatternColor::RED;
            state_.lock_.unlock();

            gVT_token_pending_ = true;
        }

        // gvt_start is always used whether this is run on a single node or a cluster
        gvt_start = now;

        // In the middle of a GVT calc, master cannot start again.
        master_can_start_ = false;

        return true;
    }

    return false;
}

bool TimeWarpMatternGVTManager::needLocalGVT() {
    if (need_local_gvt_) {
        need_local_gvt_ = false;
        return true;
    }
    return false;
}

void TimeWarpMatternGVTManager::sendMatternGVTToken(unsigned int local_minimum) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    // We shouldn't send a token, so don't
    if (!gVT_token_pending_)
        return;

    if (comm_manager_->getID() == 0) {
        global_min_ = infinityVT();  // Reset global min clock for another round
    }

    state_.lock_.lock();
    auto msg = make_unique<MatternGVTToken>(sender_id, (sender_id + 1) % num_processes,
        std::min(local_minimum, global_min_), min_red_msg_timestamp_,
        state_.white_msg_counter_ + msg_count_);
    state_.white_msg_counter_ = 0;
    state_.lock_.unlock();

    token_iteration_++;

    gVT_token_pending_ = false;

    comm_manager_->sendMessage(std::move(msg));
}

void TimeWarpMatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();

    state_.lock_.lock();

    if (process_id == 0) {
        // Initiator received the message
        if ((state_.white_msg_counter_ + msg->count == 0) && (token_iteration_ > 1)) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            sendGVTUpdate(std::min(msg->m_clock, msg->m_send));

            // Reset
            msg_count_ = 0;

        } else {
            // Accumulations
            min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
            global_min_ = infinityVT();
            msg_count_ = 0;

            // Set flags
            need_local_gvt_ = true;
            gVT_token_pending_ = true;
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (state_.color_ == MatternColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            state_.color_ = MatternColor::RED;
        }

        // Accumulations
        min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
        global_min_ = std::min(global_min_, msg->m_clock);
        msg_count_ = msg->count;

        // Set flags
        need_local_gvt_ = true;
        gVT_token_pending_ = true;
    }

    state_.lock_.unlock();
}

void TimeWarpMatternGVTManager::resetState() {
    state_.lock_.lock();
    state_.color_ = MatternColor::WHITE;
    state_.white_msg_counter_ = 0;
    state_.lock_.unlock();

    token_iteration_ = 0;
    global_min_ = infinityVT();
    min_red_msg_timestamp_ = infinityVT();

    master_can_start_ = true;
}

void TimeWarpMatternGVTManager::receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;
    gvt_updated_ = true;
}

bool TimeWarpMatternGVTManager::gvtUpdated() {
    if (gvt_updated_) {
        gvt_updated_ = false;
        return true;
    }
    return false;
}

void TimeWarpMatternGVTManager::sendGVTUpdate(unsigned int gvt) {
    // Send GVT update to all nodes including self
    for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gvt);
        comm_manager_->sendMessage(std::move(update_msg));
    }
}

} // namespace warped
