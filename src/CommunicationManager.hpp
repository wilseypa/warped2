#ifndef COMMUNICATION_MANAGER_HPP
#define COMMUNICATION_MANAGER_HPP

#include <mutex>
#include <deque>
#include <unordered_map>

#include "KernelMessage.hpp"
#include "utility/memory.hpp"

namespace warped {

class CommunicationManager {
public:
    virtual unsigned int initialize() = 0;

    virtual void finalize() = 0;

    virtual unsigned int getNumProcesses() = 0;

    virtual unsigned int getID() = 0;

    // Sends a single message
    virtual void sendMessage(std::unique_ptr<KernelMessage> msg) = 0;

    // Receives a single message
    virtual std::unique_ptr<KernelMessage> recvMessage() = 0;

    // Sends all messages that are in the send buffer
    virtual int sendAllMessages() = 0;

    void enqueueMessage(std::unique_ptr<KernelMessage> msg);
    std::unique_ptr<KernelMessage> dequeueMessage();

    bool dispatchMessages();

    void addMessageHandler(MessageType msg_type,
                           std::function<bool(std::unique_ptr<KernelMessage>)> msg_handler);

private:
    std::mutex send_queue_mutex_;
    std::deque<std::unique_ptr<KernelMessage>> send_queue_;

    std::unordered_map<int, std::function<bool(std::unique_ptr<KernelMessage>)>>
        msg_handler_by_msg_type_;

};

} // namespace warped

#endif
