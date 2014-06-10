#include "MatternGVTManager.hpp"
#include "MatternGVTControlMessage.hpp"

#include <numeric_limits>   // for infinity (std::numeric_limits<uint64_t>::max())
#include <algorithm>    // for std::min()

namespace warped {

uint64_t MatternGVTManager::infinity() {
    return std::numeric_limits<uint64_t>::max();
}

void MatternGVTManager::sendGVTUpdate() {

}

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
void MatternGVTManager::calculateGVT() {

    if (num_partitions_ > 1) {
        if (gVT_calc_initiated == false) {
            color = RED;
            min_red_message_timestamp = infinityVT();
            white_msg_counter = 0;

            sendMatternControlMessage(lVt, infinityVT(), white_msg_counter, 1);
            gVT_calc_initiated = true;
        }
    } else {
        gVT = 0;//getLastEventScheduledTime();
    {
}

void MatternGVTManager::sendMatternControlMessage(uint64_t m_clock, uint64_t m_send, uint32_t count,
                                                  uint32_t P) {

}

void MatternGVTManager::receiveMatternControlMessage(
    std::unique_ptr<MatternGVTControlMessage> msg) {

    if (node_id_ == 0) {
        // Initiator received the message
        white_msg_counter = white_msg_counter + msg->count();
        if (white_msg_counter_ == 0 && msg_round_ > 1) {
            // At this point all white messages are accounted for so we can
            // calculate the GVT now
            gVT = std::min(msg->mClock(), msg->mSend());
            gVT_calc_initiated = false;

            // Reset to white, so another calculation can be made
            white_msg_counter = 0;
            color = WHITE;
            msg_round_ = 0;

        } else {
            uint64_t T = 0;//getLastEventScheduledTime();
            sendMatternControlMessage(T, std::min(msg->mSend(), min_red_msg_timestamp_),
                                      white_msg_counter_+msg->count(), 1);
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color == WHITE) {
            min_red_msg_timestamp = infinityVT();
            color = RED;
        }

        white_msg_counter_ = white_msg_counter + msg->count();

        uint64_t T = 0;//getLastEventScheduledTime();
        sendMatternControlMessage(std::min(msg->mClock(), T),
                                  std::min(msg->mSend(), min_red_msg_timestamp_),
                                  white_msg_counter_+msg->count(), (node_id_ % num_partitions_)+1);

        white_msg_counter_ = 0;
    }
}

} // namespace warped

#endif
