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

    WARPED_REGISTER_MSG_HANDLER(TimeWarpTerminationManager, receiveTerminationToken, TerminationToken);
    WARPED_REGISTER_MSG_HANDLER(TimeWarpTerminationManager, receiveTerminator, Terminator);
}

void TimeWarpTerminationManager::sendTerminationToken(State state) {
    unsigned int sender_id = comm_manager_->getID();
    unsigned int num_processes = comm_manager_->getNumProcesses();

    if (pending_termination_token_)
        return;

    auto msg = make_unique<TerminationToken>(sender_id, (sender_id + 1) % num_processes, state);

    comm_manager_->sendMessage(std::move(msg));

    if (sender_id == 0) {
        pending_termination_token_ = true;
    }
}

void TimeWarpTerminationManager::receiveTerminationToken(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, TerminationToken>(std::move(kmsg));

    if (sticky_state_ == State::ACTIVE) {
        msg->state_ = State::ACTIVE;
    }

    if (msg->receiver_id == 0) {

        pending_termination_token_ = false;

        if (msg->state_ == State::PASSIVE) {
            // Signal termination to all nodes including self
            sendTerminator();
        }

    } else {

        sendTerminationToken(msg->state_);
    }

    sticky_state_ = state_;
}

void TimeWarpTerminationManager::sendTerminator() {
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


