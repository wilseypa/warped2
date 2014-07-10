#include "CommunicationManager.hpp"

namespace warped {

void CommunicationManager::enqueueMessage(std::unique_ptr<KernelMessage> msg) {
    send_queue_mutex_.lock();
    send_queue_.push_back(std::move(msg));
    send_queue_mutex_.unlock();
}

std::unique_ptr<KernelMessage> CommunicationManager::dequeueMessage() {
    std::unique_ptr<KernelMessage> msg = nullptr;
    send_queue_mutex_.lock();
    if (!send_queue_.empty()) {
        msg = std::move(send_queue_.front());
        send_queue_.pop_front();
    }
    send_queue_mutex_.unlock();
    return msg;
}

void CommunicationManager::addMessageHandler(MessageType msg_type,
                       std::function<MessageFlags(std::unique_ptr<KernelMessage>)> msg_handler) {
    int msg_type_int = static_cast<int>(msg_type);
    msg_handler_by_msg_type_.insert(std::make_pair(msg_type_int, msg_handler));
}

MessageFlags CommunicationManager::dispatchMessages() {
    MessageFlags ret = MessageFlags::None;
    auto msg = recvMessage();
    while (msg.get() != nullptr) {
        MessageType msg_type = msg->get_type();
        int msg_type_int = static_cast<int>(msg_type);
        auto msg_handler = msg_handler_by_msg_type_[msg_type_int];

        MessageFlags flags = msg_handler(std::move(msg));
        ret = ret | flags;

        msg = recvMessage();
    }
    return ret;
}

} // namespace warped

