#include "TerminationManager.hpp"
#include "utility/memory.hpp"   // for warped::make_unique

#include <memory> // for unique_ptr

namespace warped {

void TerminationManager::sendInitialControlMessage() {
    auto msg = make_unique<TerminationControlMessage>(TerminationState::PASSIVE);
    sendControlMessage(std::move(msg), 1);
}

void TerminationManager::sendControlMessage(std::unique_ptr<TerminationControlMessage> msg,
    uint32_t node_id) {
    msg = nullptr;
    node_id = 0;
}

void TerminationManager::receiveControlMessage(std::unique_ptr<TerminationControlMessage> msg) {

    if (sticky_state_indicator_ == TerminationState::ACTIVE) {
        msg->state = TerminationState::ACTIVE;
    }

    if (node_id_ == 0 && msg->state == TerminationState::PASSIVE) {
        terminate();
    } else if (node_id_ != 0) {
        sendControlMessage(std::move(msg), (node_id_ % num_partitions_)+1);
    } else {
        sendControlMessage(warped::make_unique<TerminationControlMessage>
            (TerminationState::PASSIVE), 1);
    }

    sticky_state_indicator_ = state_;
}

void TerminationManager::terminate() {

}

} // namespace warped

