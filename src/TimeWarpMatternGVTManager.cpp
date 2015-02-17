#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()

#include "TimeWarpMatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTUpdateMessage)

namespace warped {

void TimeWarpMatternGVTManager::initialize() {
    std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> handler =
        std::bind(&TimeWarpMatternGVTManager::receiveMatternGVTToken, this,
        std::placeholders::_1);
    comm_manager_->addRecvMessageHandler(MessageType::MatternGVTToken, handler);

    handler = std::bind(&TimeWarpMatternGVTManager::receiveGVTUpdate, this,
        std::placeholders::_1);
    comm_manager_->addRecvMessageHandler(MessageType::GVTUpdateMessage, handler);
}

unsigned int TimeWarpMatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void TimeWarpMatternGVTManager::receiveEventUpdateState(MatternColor color) {
    if (color == MatternColor::WHITE) {
        white_msg_counter_--;
    }
}

MatternColor TimeWarpMatternGVTManager::sendEventUpdateState(unsigned int timestamp) {
    if (color_ == MatternColor::WHITE) {
        white_msg_counter_++;
    } else {
        min_red_msg_timestamp_ = std::min(min_red_msg_timestamp_, timestamp);
    }
    return color_;
}

bool TimeWarpMatternGVTManager::startGVT() {

    if (comm_manager_->getID() != 0) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_start).count();

    if (gVT_token_pending_ == false && elapsed >= gvt_period_) {

        if (comm_manager_->getNumProcesses() > 1) {
            color_ = MatternColor::RED;
            min_red_msg_timestamp_ = infinityVT();
            white_msg_counter_ = 0;

            gVT_token_pending_ = true;
        }

        gvt_start = now;

        return true;
    }

    return false;
}

bool TimeWarpMatternGVTManager::completeGVT(unsigned int local_minimum) {
    bool finished = false;
    if (gVT_token_pending_) {
        sendMatternGVTToken(local_minimum);
        finished = false;
    } else {
        finished = true;
    }
    return finished;
}

void TimeWarpMatternGVTManager::sendMatternGVTToken(unsigned int local_minimum) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    unsigned int T;
    if (sender_id == 0) {
        T = local_minimum;
    } else {
        T = std::min(local_minimum, min_of_all_lvt_);
    }

    auto msg = make_unique<MatternGVTToken>(sender_id, (sender_id + 1) % num_processes,
        T, min_red_msg_timestamp_, white_msg_counter_ + msg_count_);

    token_iteration_++;
    min_of_all_lvt_ = infinityVT();
    white_msg_counter_ = 0;

    comm_manager_->sendMessage(std::move(msg));
}

void TimeWarpMatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();

    if (process_id == 0) {
        // Initiator received the message
        if ((white_msg_counter_ + msg->count == 0) && (token_iteration_ > 1)) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now

            gVT_ = std::min(msg->m_clock, msg->m_send);
            std::cout << "GVT: " << gVT_ << std::endl;
            gVT_token_pending_ = false;

            sendGVTUpdate();

            // Reset
            white_msg_counter_ = 0;
            msg_count_ = 0;
            color_ = MatternColor::WHITE;
            token_iteration_ = 0;

        } else {
            min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
            msg_count_ = 0;
            white_msg_counter_ = 0;
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternColor::RED;
        }

        min_of_all_lvt_ = std::min(min_of_all_lvt_, msg->m_clock);
        min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
        msg_count_ = msg->count;
    }
}

void TimeWarpMatternGVTManager::resetState() {
    token_iteration_ = 0;
    color_ = MatternColor::WHITE;
    gVT_token_pending_ = false;
}

void TimeWarpMatternGVTManager::receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;
}

void TimeWarpMatternGVTManager::sendGVTUpdate() {
    for (unsigned int i = 1; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gVT_);
        comm_manager_->sendMessage(std::move(update_msg));
    }
}

} // namespace warped
