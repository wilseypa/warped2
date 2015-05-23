#include <cstring> // For memset

#include "TimeWarpTerminationManager.hpp"
#include "utility/memory.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::TerminationToken)
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::Terminator)

namespace warped {

void TimeWarpTerminationManager::initialize(unsigned int num_worker_threads) {
    state_by_thread_ = make_unique<State []>(num_worker_threads);
    std::memset(state_by_thread_.get(), 0, num_worker_threads*sizeof(State));

    active_thread_count_ = num_worker_threads;

    if (comm_manager_->getID() == 0) {
        is_master_ = true;
    }

    WARPED_REGISTER_MSG_HANDLER(TimeWarpTerminationManager, receiveTerminationToken, TerminationToken);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpTerminationManager, receiveTerminator, Terminator);
}

bool TimeWarpTerminationManager::sendTerminationToken(State state, unsigned int initiator) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    // Only the current master can send a token
    if (!is_master_) {
        return false;
    }

    // When we send a token, we are not master any more.
    is_master_ = false;

    auto msg = make_unique<TerminationToken>(sender_id, (sender_id + 1) % num_processes, state, initiator);

    comm_manager_->sendMessage(std::move(msg));

    return true;
}

void TimeWarpTerminationManager::receiveTerminationToken(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, TerminationToken>(std::move(kmsg));

    // We received a token, which means we are master
    is_master_ = true;

    // If sticky state is passive, and the token has reached it's originator, then the token
    //  has circulated twice with no change state and we must terminate.
    if ((sticky_state_ == State::PASSIVE) && (msg->receiver_id == msg->initiator_)) {
        if (msg->state_ == State::PASSIVE) {
            // Signal termination to all nodes including self
            sendTerminator();
        }
        // else {
        //     remain master until this node becomes passive
        // }

    } else if (sticky_state_ == State::PASSIVE) {
        sendTerminationToken(msg->state_, msg->initiator_);
    }

    // Set sticky_state AFTER sending token
    sticky_state_ = state_;
}

void TimeWarpTerminationManager::sendTerminator() {
    // Send terminator to all nodes including self, it is now time to terminate
    for (unsigned int i = 0; i < comm_manager_->getNumProcesses(); i++) {
        auto terminator_msg = make_unique<Terminator>(0, i);
        comm_manager_->sendMessage(std::move(terminator_msg));
    }
}

void TimeWarpTerminationManager::receiveTerminator(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    // We have received a terminator message. Just set terminate_ flag
    auto msg = unique_cast<TimeWarpKernelMessage, Terminator>(std::move(kmsg));
    terminate_ = true;
}

void TimeWarpTerminationManager::setThreadPassive(unsigned int thread_id) {
    state_lock_.lock();

    if (state_by_thread_[thread_id] != State::PASSIVE) {
        state_by_thread_[thread_id] = State::PASSIVE;
        active_thread_count_--;

        if (active_thread_count_ == 0) {
            state_ = State::PASSIVE;
        }
    }

    state_lock_.unlock();
}

void TimeWarpTerminationManager::setThreadActive(unsigned int thread_id) {
    state_lock_.lock();

    if (state_by_thread_[thread_id] != State::ACTIVE) {
        state_by_thread_[thread_id] = State::ACTIVE;
        active_thread_count_++;

        state_ = State::ACTIVE;
        sticky_state_ = State::ACTIVE;
    }

    state_lock_.unlock();
}

bool TimeWarpTerminationManager::terminationStatus() {
    return (terminate_ == true);
}

bool TimeWarpTerminationManager::threadPassive(unsigned int thread_id) {
    return (state_by_thread_[thread_id] == State::PASSIVE);
}

bool TimeWarpTerminationManager::nodePassive() {
    return (state_ == State::PASSIVE);
}

} // namespace warped


