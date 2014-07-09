#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()

#include "MatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)

namespace warped {

void MatternGVTManager::initialize() {
    std::function<bool(std::unique_ptr<KernelMessage>)> handler =
        std::bind(&MatternGVTManager::receiveMatternGVTToken, this,
        std::placeholders::_1);
    comm_manager_->addMessageHandler(MessageType::MatternGVTToken, handler);

    handler = std::bind(&MatternGVTManager::receiveGVTUpdate, this,
        std::placeholders::_1);
    comm_manager_->addMessageHandler(MessageType::GVTUpdateMessage, handler);
}

unsigned int MatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void MatternGVTManager::setGvtInfo(int color) {
    if (static_cast<MatternColor>(color) == MatternColor::WHITE) {
        white_msg_counter_--;
    }
}

int MatternGVTManager::getGvtInfo(unsigned int timestamp) {
    if (color_ == MatternColor::WHITE) {
        white_msg_counter_++;
    } else {
        min_red_msg_timestamp_ = std::min(min_red_msg_timestamp_, timestamp);
    }
    return static_cast<int>(color_);
}

bool MatternGVTManager::checkGVTPeriod() {
    if (!gVT_token_pending_ ) {
        if (++gvt_period_counter_ == gvt_period_) {
            gvt_period_counter_ = 0;
            return true;
        }
    }
    return false;
}

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
bool MatternGVTManager::calculateGVT() {

    bool initiate_min_lvt = false;

    if (comm_manager_->getID() != 0) {
        return initiate_min_lvt;
    }

    if (gVT_token_pending_ == false) {
        color_ = MatternColor::RED;
        min_red_msg_timestamp_ = infinityVT();
        white_msg_counter_ = 0;

        gVT_token_pending_ = true;
        initiate_min_lvt = true;
    }

    return initiate_min_lvt;
}

void MatternGVTManager::sendMatternGVTToken(unsigned int local_minimum) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    unsigned int T;
    if (sender_id == 0) {
        T = local_minimum;
    } else {
        T = std::min(local_minimum, min_of_all_lvt_);
    }

    auto msg = make_unique<MatternGVTToken>(sender_id, (sender_id + 1) % num_processes,
        T, min_red_msg_timestamp_, white_msg_counter_);

    white_msg_counter_ = 0;

    comm_manager_->sendMessage(std::move(msg));
}

bool MatternGVTManager::receiveMatternGVTToken(std::unique_ptr<KernelMessage> kmsg) {
    auto msg = unique_cast<KernelMessage, MatternGVTToken>(std::move(kmsg));
    unsigned int process_id = comm_manager_->getID();
    bool initiate_min_lvt = false;

    if (process_id == 0) {
        // Initiator received the message
        if (white_msg_counter_ + msg->count == 0) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            gVT_ = std::min(msg->m_clock, msg->m_send);
            gVT_token_pending_ = false;

            sendGVTUpdate();

            // Reset to white, so another calculation can be made
            white_msg_counter_ = 0;
            color_ = MatternColor::WHITE;

        } else {
            min_red_msg_timestamp_ = std::min(msg->m_send, min_red_msg_timestamp_);
            initiate_min_lvt = true;
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternColor::RED;
        }

        min_of_all_lvt_ = std::min(min_of_all_lvt_, msg->m_clock);
        min_red_msg_timestamp_ = std::min(msg->m_clock, min_red_msg_timestamp_);
        white_msg_counter_ = white_msg_counter_ + msg->count;

        initiate_min_lvt = true;
    }
    return initiate_min_lvt;
}

} // namespace warped

