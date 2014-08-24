#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()

#include "TimeWarpMatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)

namespace warped {

void TimeWarpMatternGVTManager::initialize() {
    std::function<MessageFlags(std::unique_ptr<TimeWarpKernelMessage>)> handler =
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

void TimeWarpMatternGVTManager::receiveEventUpdate(MatternColor color) {
    if (color == MatternColor::WHITE) {
        white_msg_counter_--;
    }
}

MatternColor TimeWarpMatternGVTManager::sendEventUpdate(unsigned int timestamp) {
    if (color_ == MatternColor::WHITE) {
        white_msg_counter_++;
    } else {
        min_red_msg_timestamp_ = std::min(min_red_msg_timestamp_, timestamp);
    }
    return color_;
}

MessageFlags TimeWarpMatternGVTManager::calculateGVT() {

    MessageFlags flags = MessageFlags::None;

    if (comm_manager_->getID() != 0) {
        return flags;
    }

    if (gVT_token_pending_ == false) {
        color_ = MatternColor::RED;
        min_red_msg_timestamp_ = infinityVT();
        white_msg_counter_ = 0;

        gVT_token_pending_ = true;
        flags = MessageFlags::PendingMatternToken;
    }

    return flags;
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

MessageFlags TimeWarpMatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<TimeWarpKernelMessage> kmsg) {

    auto msg = unique_cast<TimeWarpKernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();
    MessageFlags flags = MessageFlags::None;

    if (process_id == 0) {
        // Initiator received the message
        if ((white_msg_counter_ + msg->count == 0) && (token_iteration_ > 1)) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now

            gVT_ = std::min(msg->m_clock, msg->m_send);
            std::cout << "GVT: " << gVT_ << std::endl;
            gVT_token_pending_ = false;

            sendGVTUpdate();
            flags |= MessageFlags::GVTUpdate;

            // Reset
            white_msg_counter_ = 0;
            msg_count_ = 0;
            color_ = MatternColor::WHITE;
            token_iteration_ = 0;

        } else {
            min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
            msg_count_ = 0;
            white_msg_counter_ = 0;
            flags |= MessageFlags::PendingMatternToken;
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

        flags |= MessageFlags::PendingMatternToken;
    }
    return flags;
}

void TimeWarpMatternGVTManager::reset() {
    token_iteration_ = 0;
    color_ = MatternColor::WHITE;
    gVT_token_pending_ = false;
}

} // namespace warped
