#include <algorithm>    // for std::min()
#include <cassert>

#include "TimeWarpAsynchronousGVTManager.hpp"
#include "utility/warnings.hpp"
#include "utility/memory.hpp"           // for make_unique

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::MatternGVTToken)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTUpdateMessage)

namespace warped {

void TimeWarpAsynchronousGVTManager::initialize() {
    WARPED_REGISTER_MSG_HANDLER(TimeWarpAsynchronousGVTManager, receiveMatternGVTToken, MatternGVTToken);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpAsynchronousGVTManager, receiveGVTUpdate, GVTUpdateMessage);

    // Prepare local min lvt computation
    local_min_ = make_unique<unsigned int []>(num_worker_threads_+1);
    send_min_ = make_unique<unsigned int []>(num_worker_threads_+1);
    calculated_min_flag_ = make_unique<bool []>(num_worker_threads_+1);

    resetLocalState();

    TimeWarpGVTManager::initialize();
}

bool TimeWarpAsynchronousGVTManager::readyToStart() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - gvt_start).count();
    return ((elapsed >= gvt_period_) && (comm_manager_->getID() == 0));
}

void TimeWarpAsynchronousGVTManager::progressGVT() {

    if (gvt_state_ == GVTState::LOCAL) {

        // If we need a local gvt calculation and haven't started yet
        if ((local_gvt_flag_.load() == 0) && !started_local_gvt_) {

            if (!started_global_gvt_ && (comm_manager_->getID() == 0)) {
                state_.lock_.lock();

                state_.min_timestamp_ = (unsigned int)-1;

                assert(state_.color_ == state_.initial_color_);
                toggleColor();

                msg_count_ = initialColorCount();
                initialColorCount() = 0;

                state_.lock_.unlock();

                started_global_gvt_ = true;

            }

            resetLocalState();
            local_gvt_flag_.store(num_worker_threads_);
            started_local_gvt_ = true;

        // If we have finished local gvt calculation
        } else if ((local_gvt_flag_.load() == 0) && started_local_gvt_) {

            unsigned int min = std::numeric_limits<unsigned int>::max();
            for (unsigned int i = 0; i <= num_worker_threads_; i++) {
                min = std::min(min, local_min_[i]);
            }

            if (comm_manager_->getNumProcesses() > 1) {
                gvt_state_ = GVTState::GLOBAL;
                sendMatternGVTToken(min);
            } else {
                gvt_state_ = GVTState::IDLE;
                gVT_ = min;
                gvt_updated_ = true;
            }

            started_local_gvt_ = false;
        }
    }
}

Color TimeWarpAsynchronousGVTManager::sendEventUpdate(std::shared_ptr<Event>& event) {
    state_.lock_.lock();

    auto color = state_.color_;

    if (color == Color::WHITE) {
        state_.white_msg_count_++;
    } else {
        state_.red_msg_count_++;
    }

    if (color != state_.initial_color_) {
        state_.min_timestamp_ = std::min(state_.min_timestamp_, event->timestamp());
    }

    state_.lock_.unlock();

    return color;
}

void TimeWarpAsynchronousGVTManager::receiveEventUpdate(std::shared_ptr<Event>& event, Color color) {
    unused(event);

    state_.lock_.lock();
    if (color == Color::WHITE) {
        state_.white_msg_count_--;
    } else {
        state_.red_msg_count_--;
    }
    state_.lock_.unlock();
}

void TimeWarpAsynchronousGVTManager::toggleColor() {
    state_.color_ = (state_.color_ == Color::WHITE) ? Color::RED : Color::WHITE;
}

void TimeWarpAsynchronousGVTManager::toggleInitialColor() {
    state_.initial_color_ = (state_.initial_color_ == Color::WHITE) ? Color::RED : Color::WHITE;
}

int& TimeWarpAsynchronousGVTManager::initialColorCount() {
    return (state_.initial_color_ == Color::RED) ? state_.red_msg_count_ : state_.white_msg_count_;
}

void TimeWarpAsynchronousGVTManager::sendMatternGVTToken(unsigned int local_minimum) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

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
    comm_manager_->flushMessages();
}

void TimeWarpAsynchronousGVTManager::receiveMatternGVTToken(
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

            gvt_state_ = GVTState::GLOBAL;
            started_global_gvt_ = false;

        } else {
            // Account for minumum red msg timestamp from nodes that the token has visited so far
            state_.min_timestamp_ = std::min(msg->m_send, state_.min_timestamp_);

            // Hold the white message count so it can be used when the token is sent
            msg_count_ = initialColorCount() + msg->count;
            initialColorCount() = 0;

            gvt_state_ = GVTState::LOCAL;
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

        gvt_state_ = GVTState::LOCAL;
    }

    state_.lock_.unlock();
}

void TimeWarpAsynchronousGVTManager::receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;

    global_min_clock_ = (unsigned int)-1;
    toggleInitialColor();

    if (gvt_state_ == GVTState::GLOBAL) {
        gvt_state_ = GVTState::IDLE;
    }

    gvt_updated_ = true;
}

bool TimeWarpAsynchronousGVTManager::gvtUpdated() {
    if (gvt_updated_) {
        gvt_updated_ = false;
        return true;
    }
    return false;
}

void TimeWarpAsynchronousGVTManager::sendGVTUpdate(unsigned int gvt) {
    // Send GVT update to all nodes including self
    for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gvt);
        comm_manager_->insertMessage(std::move(update_msg));
    }
}

void TimeWarpAsynchronousGVTManager::resetLocalState() {
    for (unsigned int i = 0; i < num_worker_threads_+1; i++) {
        // Reset send_min back to very large number for next calculation
        send_min_[i] = std::numeric_limits<unsigned int>::max();
        calculated_min_flag_[i] = false;
        local_min_[i] = std::numeric_limits<unsigned int>::max();
    }

    // Manager thread will not receive any events
    calculated_min_flag_[num_worker_threads_] = true;
}


void TimeWarpAsynchronousGVTManager::reportThreadMin(unsigned int timestamp, unsigned int thread_id,
                                                     unsigned int local_gvt_flag) {
    // If this thread has not yet reported a minimum, then report it.
    if (local_gvt_flag > 0 && !calculated_min_flag_[thread_id]) {
        local_min_[thread_id] = std::min(send_min_[thread_id], timestamp);
        calculated_min_flag_[thread_id] = true;
        local_gvt_flag_.fetch_sub(1);
    }
}

void TimeWarpAsynchronousGVTManager::reportThreadSendMin(unsigned int timestamp, unsigned int thread_id) {
    // If this thread has not yet reported a minimum, make sure to report
    //  any sends to avoid the "simultaneous reporting problem"
    if (local_gvt_flag_.load() > 0 && !calculated_min_flag_[thread_id]) {
        send_min_[thread_id] = std::min(send_min_[thread_id], timestamp);
    }
}

unsigned int TimeWarpAsynchronousGVTManager::getLocalGVTFlag() {
    return local_gvt_flag_.load();
}

} // namespace warped
