#include "TimeWarpCommunicationManager.hpp"

namespace warped {

void TimeWarpCommunicationManager::addRecvMessageHandler(MessageType msg_type,
    std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler) {

    int msg_type_int = static_cast<int>(msg_type);
    msg_handler_by_msg_type_.insert(std::make_pair(msg_type_int, msg_handler));
}

void TimeWarpCommunicationManager::deliverReceivedMessages() {
    auto msg = getMessage();
    while (msg.get() != nullptr) {
        MessageType msg_type = msg->get_type();
        int msg_type_int = static_cast<int>(msg_type);

        auto msg_handler = msg_handler_by_msg_type_[msg_type_int];
        msg_handler(std::move(msg));

        msg = getMessage();
    }
}

} // namespace warped

