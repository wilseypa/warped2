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

MatternColor MatternNodeState::color_;
int MatternNodeState::white_msg_counter_;
unsigned int MatternNodeState::min_red_msg_timestamp_;
std::mutex MatternNodeState::lock_;

void TimeWarpMatternGVTManager::initialize() {
    WARPED_REGISTER_MSG_HANDLER(TimeWarpMatternGVTManager, receiveMatternGVTToken, MatternGVTToken);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpMatternGVTManager, receiveGVTUpdate, GVTUpdateMessage);
    gvt_start = std::chrono::steady_clock::now();
}

unsigned int TimeWarpMatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void TimeWarpMatternGVTManager::receiveEventUpdateState(MatternColor color) {
    MatternNodeState::lock_.lock();
    if (color == MatternColor::WHITE) {
        MatternNodeState::white_msg_counter_--;
    }
    MatternNodeState::lock_.unlock();
}

MatternColor TimeWarpMatternGVTManager::sendEventUpdateState(unsigned int timestamp) {
    MatternNodeState::lock_.lock();

    if (MatternNodeState::color_ == MatternColor::WHITE) {
        MatternNodeState::white_msg_counter_++;
    } else {
        MatternNodeState::min_red_msg_timestamp_ =
            std::min(MatternNodeState::min_red_msg_timestamp_, timestamp);
    }

    MatternColor color = MatternNodeState::color_;

    MatternNodeState::lock_.unlock();

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
            MatternNodeState::lock_.lock();
            assert(MatternNodeState::color_ == MatternColor::WHITE);
            MatternNodeState::color_ = MatternColor::RED;
            MatternNodeState::lock_.unlock();

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
        global_min_clock_ = infinityVT();  // Reset global min clock for another round
    }

    MatternNodeState::lock_.lock();
    auto msg = make_unique<MatternGVTToken>(sender_id, (sender_id + 1) % num_processes,
        std::min(local_minimum, global_min_clock_), MatternNodeState::min_red_msg_timestamp_,
        MatternNodeState::white_msg_counter_ + msg_count_);
    MatternNodeState::white_msg_counter_ = 0;
    MatternNodeState::lock_.unlock();

    token_iteration_++;

    comm_manager_->insertMessage(std::move(msg));
    gVT_token_pending_ = false;

}

void TimeWarpMatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();

    MatternNodeState::lock_.lock();

    if (process_id == 0) {
        // Initiator received the message
        if ((MatternNodeState::white_msg_counter_ + msg->count == 0) && (token_iteration_ > 1)) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            sendGVTUpdate(std::min(msg->m_clock, msg->m_send));

            // Reset
            MatternNodeState::white_msg_counter_ = 0;

        } else {
            // Accumulations
            MatternNodeState::min_red_msg_timestamp_ = std::min(msg->m_send, MatternNodeState::min_red_msg_timestamp_);
            global_min_clock_ = infinityVT();
            MatternNodeState::white_msg_counter_ = 0;

            // Set flags
            need_local_gvt_ = true;
            gVT_token_pending_ = true;
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (MatternNodeState::color_ == MatternColor::WHITE) {
            MatternNodeState::min_red_msg_timestamp_ = infinityVT();
            MatternNodeState::color_ = MatternColor::RED;
        }

        // Accumulations
        MatternNodeState::min_red_msg_timestamp_ = std::min(msg->m_send, MatternNodeState::min_red_msg_timestamp_);
        global_min_clock_ = std::min(global_min_clock_, msg->m_clock);

        // Hold count from message
        msg_count_ = msg->count;

        // Set flags
        need_local_gvt_ = true;
        gVT_token_pending_ = true;
    }

    MatternNodeState::lock_.unlock();
}

void TimeWarpMatternGVTManager::resetState() {
    MatternNodeState::lock_.lock();
    MatternNodeState::color_ = MatternColor::WHITE;
    MatternNodeState::white_msg_counter_ = 0;
    MatternNodeState::min_red_msg_timestamp_ = infinityVT();
    MatternNodeState::lock_.unlock();

    token_iteration_ = 0;
    global_min_clock_ = infinityVT();

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
        comm_manager_->insertMessage(std::move(update_msg));
    }
}

} // namespace warped
