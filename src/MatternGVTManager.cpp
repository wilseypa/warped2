#include "MatternGVTManager.hpp"

#include <limits>   // for infinity (std::numeric_limits<uint64_t>::max())
#include <algorithm>    // for std::min()

#include "utility/memory.hpp"

namespace warped {

uint64_t MatternGVTManager::infinityVT() {
    return std::numeric_limits<uint64_t>::max();
}

void MatternGVTManager::sendGVTUpdate() {

}

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
void MatternGVTManager::calculateGVT() {

    if (num_partitions_ > 1) {
        if (gVT_calc_initiated_ == false) {
            color_ = MatternMsgColor::RED;
            min_red_msg_timestamp_ = infinityVT();
            white_msg_counter_ = 0;

            uint64_t T = 0;//getLastEventScheduledTime();
            sendMatternControlMessage(warped::make_unique<MatternGVTControlMessage>(T,
                infinityVT(), white_msg_counter_) , 1);
            gVT_calc_initiated_ = true;
        }
    } else {
        gVT_ = 0;//getLastEventScheduledTime();
    }
}

void MatternGVTManager::sendMatternControlMessage(
    std::unique_ptr<MatternGVTControlMessage> msg, uint32_t P) {
    //TODO
    msg = nullptr;
    P = 0;
}

void MatternGVTManager::receiveMatternControlMessage(
    std::unique_ptr<MatternGVTControlMessage> msg) {

    if (node_id_ == 0) {
        // Initiator received the message
        white_msg_counter_ = white_msg_counter_ + msg->count;
        if (white_msg_counter_ == 0 && msg_round_ > 1) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            gVT_ = std::min(msg->m_clock, msg->m_send);
            gVT_calc_initiated_ = false;

            // Reset to white, so another calculation can be made
            white_msg_counter_ = 0;
            color_ = MatternMsgColor::WHITE;
            msg_round_ = 0;

        } else {
            uint64_t T = 0;//getLastEventScheduledTime();
            sendMatternControlMessage(warped::make_unique<MatternGVTControlMessage>(T,
                std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count) , 1);
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternMsgColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternMsgColor::RED;
        }

        white_msg_counter_ = white_msg_counter_ + msg->count;

        uint64_t T = 0;//getLastEventScheduledTime();
        sendMatternControlMessage(warped::make_unique<MatternGVTControlMessage>
            (std::min(msg->m_clock, T), std::min(msg->m_send, min_red_msg_timestamp_),
            white_msg_counter_+msg->count), (node_id_ % num_partitions_)+1);

        white_msg_counter_ = 0;
    }
}

} // namespace warped

