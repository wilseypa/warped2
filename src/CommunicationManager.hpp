#ifndef COMMUNICATION_MANAGER_HPP
#define COMMUNICATION_MANAGER_HPP

#include <mutex>
#include <deque>

#include "KernelMessage.hpp"

namespace warped {

class CommunicationManager {
public:
    virtual void initialize() = 0;

    virtual void finalize() = 0;

    virtual unsigned int getNumProcesses() = 0;

    virtual unsigned int getID() = 0;

    virtual void sendMessage(std::unique_ptr<KernelMessage> msg) = 0;

    virtual std::unique_ptr<KernelMessage> recvMessage() = 0;

    virtual int sendAllMessages() = 0;

    void enqueueMessage(std::unique_ptr<KernelMessage> msg);

    std::unique_ptr<KernelMessage> dequeueMessage();

private:
    std::mutex send_queue_mutex_;
    std::deque<std::unique_ptr<KernelMessage>> send_queue_;
};

} // namespace warped

#endif
