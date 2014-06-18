#include <limits>   // for infinity (std::numeric_limits<>::max())
#include <algorithm>    // for std::min()

#include "MatternGVTManager.hpp"
#include "utility/memory.hpp"           // for make_unique

/*  This class implements the Mattern GVT algorithm and provides methods
 *  for initiating a calculation cycle and for processing received tokens
 */

namespace warped {

unsigned int MatternGVTManager::infinityVT() {
    return std::numeric_limits<unsigned int>::max();
}

void MatternGVTManager::sendGVTUpdate() {

}

// This initiates the gvt calculation by sending the initial
// control message to node 1 (assuming this must be node 0 calling this)
void MatternGVTManager::calculateGVT(std::function<unsigned int()> getMinimumLVT,
    MPICommunicationManager *mpi_manager) {

    if (mpi_manager->getID() != 0) return;

    if (mpi_manager->getNumProcesses() > 1) {
        if (gVT_calc_initiated_ == false) {
            color_ = MatternMsgColor::RED;
            min_red_msg_timestamp_ = infinityVT();
            white_msg_counter_ = 0;

            unsigned int T = getMinimumLVT();
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(0, 1, T, infinityVT(),
                white_msg_counter_), mpi_manager);
            gVT_calc_initiated_ = true;
        }
    } else {
        gVT_ = getMinimumLVT();
    }
}

void MatternGVTManager::sendMatternGVTToken(std::unique_ptr<MatternGVTToken> msg,
    MPICommunicationManager *mpi_manager) {
    mpi_manager->sendMessage(std::move(msg));
}

void MatternGVTManager::receiveMatternGVTToken(std::unique_ptr<MatternGVTToken> msg,
    std::function<unsigned int()> getMinimumLVT, MPICommunicationManager *mpi_manager) {

    unsigned int process_id = mpi_manager->getID();
    unsigned int num_processes = mpi_manager->getNumProcesses();

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
            color_ = MatternMsgColor::WHITE;

        } else {
            unsigned int T = getMinimumLVT();
            // count is not zero so start another round
            sendMatternGVTToken(warped::make_unique<MatternGVTToken>(0, 1, T,
                std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count),
                mpi_manager);
        }

    } else {
        // A node other than the initiator is now receiving a control message
        if (color_ == MatternMsgColor::WHITE) {
            min_red_msg_timestamp_ = infinityVT();
            color_ = MatternMsgColor::RED;
        }

        unsigned int T = getMinimumLVT();
        // Pass the token on to the next node in the logical ring
        sendMatternGVTToken(warped::make_unique<MatternGVTToken>(process_id,
            (process_id % num_processes)+1, std::min(msg->m_clock, T),
            std::min(msg->m_send, min_red_msg_timestamp_), white_msg_counter_+msg->count),
            mpi_manager);

        // Must be reset for next round
        white_msg_counter_ = 0;
    }
}

} // namespace warped

