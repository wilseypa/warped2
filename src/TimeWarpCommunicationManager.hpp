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
 * processes(nodes), get a node id, send a single message, and receive a single message.
*/

namespace warped {

class TimeWarpCommunicationManager {
public:
    virtual unsigned int initialize() = 0;

    virtual void finalize() = 0;

    virtual unsigned int getNumProcesses() = 0;

    virtual unsigned int getID() = 0;

    virtual int waitForAllProcesses() = 0;

    virtual int sumReduceUint(const unsigned int* send_local, unsigned int* recv_global) = 0;

    // Sends a single message
    virtual void sendMessage(std::unique_ptr<TimeWarpKernelMessage> msg) = 0;

    // Receives a single message
    virtual std::unique_ptr<TimeWarpKernelMessage> recvMessage() = 0;

    // Passes messages to the correct message handler
    void dispatchReceivedMessages();

    // Adds a MessageType/Message handler pair for dispatching messages
    void addRecvMessageHandler(MessageType msg_type,
        std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler);

private:
    // Map to lookup message handler given a message type
    std::unordered_map<int, std::function<void(std::unique_ptr<TimeWarpKernelMessage>)>>
        msg_handler_by_msg_type_;

};

} // namespace warped

#endif
