#ifndef COMMUNICATION_MANAGER_HPP
#define COMMUNICATION_MANAGER_HPP

#include <mutex>
#include <deque>
#include <unordered_map>
#include <cstdint> // for uint8_t

#include "KernelMessage.hpp"
#include "utility/memory.hpp"

namespace warped {

enum class MessageFlags : uint8_t {
    None = 0,
    PendingMatternToken = 1 << 0, // 1
    GVTUpdate = 1 << 1 // 2
};

#define PENDING_MATTERN_TOKEN(x) ((MessageFlags::PendingMatternToken & x) == \
    MessageFlags::PendingMatternToken)

#define GVT_UPDATE(x) ((MessageFlags::GVTUpdate & x) == MessageFlags::GVTUpdate)

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

    MessageFlags dispatchMessages();

    void addMessageHandler(MessageType msg_type,
                           std::function<MessageFlags(std::unique_ptr<KernelMessage>)> msg_handler);

private:
    std::mutex send_queue_mutex_;
    std::deque<std::unique_ptr<KernelMessage>> send_queue_;

    std::unordered_map<int, std::function<MessageFlags(std::unique_ptr<KernelMessage>)>>
        msg_handler_by_msg_type_;

};

inline std::underlying_type<MessageFlags>::type operator*(MessageFlags val)
{
    return static_cast<std::underlying_type<MessageFlags>::type>(val);
}

inline MessageFlags operator| (MessageFlags a, MessageFlags b) {
    return static_cast<MessageFlags>((*a)|(*b));
}

inline MessageFlags operator& (MessageFlags a, MessageFlags b) {
    return static_cast<MessageFlags>((*a)&(*b));
}

inline MessageFlags operator|= (MessageFlags a, MessageFlags b) {
    return a = a | b;
}

inline MessageFlags operator&= (MessageFlags a, MessageFlags b) {
    return a = a & b;
}

inline MessageFlags operator~ (MessageFlags a) {
    return static_cast<MessageFlags>(~(*a));
}

} // namespace warped

#endif
