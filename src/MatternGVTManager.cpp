#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()

#include "MatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)

namespace warped {

unsigned int MatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void MatternGVTManager::sendGVTUpdate() {

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

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
void MatternGVTManager::calculateGVT() {

    if (comm_manager_->getID() != 0) return;

    if (comm_manager_->getNumProcesses() > 1) {
        if (gVT_calc_initiated_ == false) {
            color_ = MatternColor::RED;
            min_red_msg_timestamp_ = infinityVT();
            white_msg_counter_ = 0;

            unsigned int T = 0; //getMinimumLVT();
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(0, 1, T, infinityVT(),
                white_msg_counter_));
            gVT_calc_initiated_ = true;
        }
    } else {
        gVT_ = 0; //getMinimumLVT();
    }
}

void MatternGVTManager::sendMatternGVTToken(std::unique_ptr<MatternGVTToken> msg) {
    comm_manager_->sendMessage(std::move(msg));
}

void MatternGVTManager::receiveMessage(std::unique_ptr<KernelMessage> msg) {
    if (msg->get_type() == MessageType::MatternGVTToken) {
        MatternGVTToken *m = dynamic_cast<MatternGVTToken*>(msg.get());
        std::unique_ptr<MatternGVTToken> token;
        if(m != nullptr)
        {
            msg.release();
            token.reset(m);
        }
        receiveMatternGVTToken(std::move(token));
    }
}

void MatternGVTManager::receiveMatternGVTToken(std::unique_ptr<MatternGVTToken> msg) {
    unsigned int process_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    if (process_id == 0) {
        // Initiator received the message
        if (white_msg_counter_ + msg->count == 0) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            gVT_ = std::min(msg->m_clock, msg->m_send);
            gVT_calc_initiated_ = false;

            sendGVTUpdate();

            // Reset to white, so another calculation can be made
            white_msg_counter_ = 0;
            color_ = MatternColor::WHITE;

        } else {
            unsigned int T = 0;//getMinimumLVT();
            // count is not zero so start another round
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(0, 1, T,
                std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count));
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternColor::RED;
        }

        unsigned int T = 0;//getMinimumLVT();
        // Pass the token on to the next node in the logical ring
        sendMatternGVTToken(warped::make_unique<MatternGVTToken>(process_id,
            (process_id % num_processes)+1, std::min(msg->m_clock, T),
            std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count));

        // Must be reset for next round
        white_msg_counter_ = 0;
    }
}

} // namespace warped

