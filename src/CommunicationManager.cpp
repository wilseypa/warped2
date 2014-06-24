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

} // namespace warped

