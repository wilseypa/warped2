#ifndef COMMUNICATION_MANAGER_HPP
#define COMMUNICATION_MANAGER_HPP

#include <mutex>
#include <deque>
#include <unordered_map>
#include <cstdint> // for uint8_t

#include "TimeWarpKernelMessage.hpp"
#include "utility/memory.hpp"

/* This class is a base class for any specific communication protocol. Any subclass must
 * implement methods to initialize communication, finalize communication, get the number of
 * processes(nodes), get a node id, send a single message, receive a single message, and
 * send all messages. This class also contains a send queue of messages, and methods to enqueue
 * and dequeue.
 *
 * Any class that wants to communicate should have a shared pointer to an object of this class and
 * must add a message handler for any message type that it handles in an initialization routine.
 * The message handler must have a return type of MessageFlags and be passed a unique pointer to
 * a TimeWarpKernelMessage.
 */

namespace warped {

enum class MessageFlags : uint8_t {
    None = 0,
    PendingMatternToken = 1 << 0, // 1
    GVTUpdate = 1 << 1 // 2
};

#define PENDING_MATTERN_TOKEN(x) ((MessageFlags::PendingMatternToken & x) == \
    MessageFlags::PendingMatternToken)

#define GVT_UPDATE(x) ((MessageFlags::GVTUpdate & x) == MessageFlags::GVTUpdate)

class TimeWarpCommunicationManager {
public:
    virtual unsigned int initialize() = 0;

    virtual void finalize() = 0;

    virtual unsigned int getNumProcesses() = 0;

    virtual unsigned int getID() = 0;

    // Sends a single message
    virtual void sendMessage(std::unique_ptr<TimeWarpKernelMessage> msg) = 0;

    // Receives a single message
    virtual std::unique_ptr<TimeWarpKernelMessage> recvMessage() = 0;

    // Sends all messages that are in the send buffer
    virtual int sendAllMessages() = 0;

    // Enqueues a message into the send queue
    void enqueueMessage(std::unique_ptr<TimeWarpKernelMessage> msg);

    // dequeues a message from the send queue
    std::unique_ptr<TimeWarpKernelMessage> dequeueMessage();

    // Passes messages to the correct message handler
    MessageFlags dispatchMessages();

    // Adds a MessageType/Message handler pair for dispatching messages
    void addMessageHandler(MessageType msg_type,
        std::function<MessageFlags(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler);

private:
    // lock the the send queue, worker threads will need to enqueue, manager thread will dequeue
    std::mutex send_queue_mutex_;

    // Send queue
    std::deque<std::unique_ptr<TimeWarpKernelMessage>> send_queue_;

    // Map to lookup message handler given a message type
    std::unordered_map<int, std::function<MessageFlags(std::unique_ptr<TimeWarpKernelMessage>)>>
        msg_handler_by_msg_type_;

};

// MessageFlags operators
///////////////////////////////////////////////////////////////////////////
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

inline MessageFlags& operator|= (MessageFlags& a, MessageFlags b) {
    return a = a | b;
}

inline MessageFlags operator&= (MessageFlags& a, MessageFlags b) {
    return a = a & b;
}

inline MessageFlags operator~ (MessageFlags a) {
    return static_cast<MessageFlags>(~(*a));
}

} // namespace warped

#endif
