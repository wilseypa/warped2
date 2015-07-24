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

MatternColor TimeWarpMatternGVTManager::sendUpdate(unsigned int timestamp) {
    state_.lock_.lock();

    auto color = state_.color_;

    if (color == MatternColor::WHITE) {
        state_.white_msg_count_++;
    } else {
        state_.red_msg_count_++;
    }

    if (color != state_.initial_color_) {
        state_.min_timestamp_ = std::min(state_.min_timestamp_, timestamp);
    }

    state_.lock_.unlock();

    return color;
}

void TimeWarpMatternGVTManager::receiveUpdate(MatternColor color) {
    state_.lock_.lock();
    if (color == MatternColor::WHITE) {
        state_.white_msg_count_--;
    } else {
        state_.red_msg_count_--;
    }
    state_.lock_.unlock();
}

void TimeWarpMatternGVTManager::toggleColor() {
    state_.color_ = (state_.color_ == MatternColor::WHITE) ? MatternColor::RED : MatternColor::WHITE;
}

void TimeWarpMatternGVTManager::toggleInitialColor() {
    state_.initial_color_ = (state_.initial_color_ == MatternColor::WHITE) ? MatternColor::RED : MatternColor::WHITE;
}

int& TimeWarpMatternGVTManager::initialColorCount() {
    return (state_.initial_color_ == MatternColor::RED) ? state_.red_msg_count_ : state_.white_msg_count_;
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

            assert(state_.color_ == state_.initial_color_);
            toggleColor();

            state_.min_timestamp_ = (unsigned int)-1;
            global_min_clock_ = (unsigned int)-1;

            msg_count_ = initialColorCount();
            initialColorCount() = 0;

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
        global_min_clock_ = (unsigned int)-1;  // Reset global min clock for another round
    }

    state_.lock_.lock();

    auto msg = make_unique<MatternGVTToken>(
        sender_id,                                      // Sender
        (sender_id + 1) % num_processes,                // Receiver
        std::min(local_minimum, global_min_clock_),     // Accumulated minimum clock nodes
        state_.min_timestamp_,                          // Accumulated Minimum red messages
        msg_count_);                                    // Accumulated white msg count

    state_.lock_.unlock();

    comm_manager_->insertMessage(std::move(msg));

    gVT_token_pending_ = false;

}

void TimeWarpMatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();

    state_.lock_.lock();

    if (process_id == 0) {
        // Initiator received the message
        if (msg->count <= 0) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            sendGVTUpdate(std::min(msg->m_clock, msg->m_send));

        } else {
            // Account for minumum red msg timestamp from nodes that the token has visited so far
            state_.min_timestamp_ = std::min(msg->m_send, state_.min_timestamp_);

            // Hold the white message count so it can be used when the token is sent
            msg_count_ = initialColorCount() + msg->count;
            initialColorCount() = 0;

            // Set flags
            need_local_gvt_ = true;
            gVT_token_pending_ = true;
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (state_.color_ == state_.initial_color_) {
            state_.min_timestamp_ = (unsigned int)-1;
            toggleColor();
        }

        // Accumulations
        state_.min_timestamp_ = std::min(msg->m_send, state_.min_timestamp_);
        global_min_clock_ = std::min(global_min_clock_, msg->m_clock);

        // Hold count from message
        msg_count_ = initialColorCount() + msg->count;
        initialColorCount() = 0;

        // Set flags
        need_local_gvt_ = true;
        gVT_token_pending_ = true;
    }

    state_.lock_.unlock();
}

void TimeWarpMatternGVTManager::receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;

    global_min_clock_ = (unsigned int)-1;
    toggleInitialColor();
    master_can_start_ = true;

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
        comm_manager_->insertMessage(std::move(update_msg));
    }
}

} // namespace warped
