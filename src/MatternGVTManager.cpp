#include <limits>   // for infinity (std::numeric_limits<uint64_t>::max())
#include <algorithm>    // for std::min()

#include "MatternGVTManager.hpp"
#include "utility/memory.hpp"
#include "TimeWarpEventDispatcher.hpp"

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

namespace warped {

uint64_t MatternGVTManager::infinityVT() {
    return std::numeric_limits<uint64_t>::max();
}

void MatternGVTManager::sendGVTUpdate() {

}

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
void MatternGVTManager::calculateGVT() {

    if (num_nodes_ > 1) {
        if (gVT_calc_initiated_ == false) {
            color_ = MatternMsgColor::RED;
            min_red_msg_timestamp_ = infinityVT();
            white_msg_counter_ = 0;

            uint64_t T = TimeWarpEventDispatcher::getMinimumLVT();
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(T, infinityVT(),
                white_msg_counter_) , 1);
            gVT_calc_initiated_ = true;
        }
    } else {
        gVT_ = TimeWarpEventDispatcher::getMinimumLVT();
    }
}

void MatternGVTManager::sendMatternGVTToken(
    std::unique_ptr<MatternGVTToken> msg, uint32_t P) {
    //TODO
    msg = nullptr;
    P = 0;
}

void MatternGVTManager::receiveMatternGVTToken(
    std::unique_ptr<MatternGVTToken> msg) {

    if (node_id_ == 0) {
        // Initiator received the message
        if (white_msg_counter_ + msg->count == 0) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            gVT_ = std::min(msg->m_clock, msg->m_send);
            gVT_calc_initiated_ = false;

            // Reset to white, so another calculation can be made
            white_msg_counter_ = 0;
            color_ = MatternMsgColor::WHITE;

        } else {
            uint64_t T = TimeWarpEventDispatcher::getMinimumLVT();
            // count is not zero so start another round
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(T,
                std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count) , 1);
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternMsgColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternMsgColor::RED;
        }

        uint64_t T = TimeWarpEventDispatcher::getMinimumLVT();
        // Pass the token on to the next node in the logical ring
        sendMatternGVTToken(warped::make_unique<MatternGVTToken>
            (std::min(msg->m_clock, T), std::min(msg->m_send, min_red_msg_timestamp_),
            white_msg_counter_+msg->count), (node_id_ % num_nodes_)+1);

        // Must be reset for next round
        white_msg_counter_ = 0;
    }
}

} // namespace warped

