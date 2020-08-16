#ifndef COMMUNICATION_MANAGER_HPP
#define COMMUNICATION_MANAGER_HPP

#include <mutex>
#include <deque>
#include <unordered_map>
#include <cstdint> // for uint8_t

#include "LogicalProcess.hpp"
#include "TimeWarpKernelMessage.hpp"
#include "utility/memory.hpp"

/* This class is a base class for any specific communication protocol. Any subclass must
 * implement methods to initialize communication, finalize communication, get the number of
 * processes(nodes), get a node id, send a single message, and receive a single message.
*/

#define WARPED_REGISTER_MSG_HANDLER(class, func, msg_type) {\
    std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> handler = \
        std::bind(&class::func, this, std::placeholders::_1);\
    comm_manager_->addRecvMessageHandler(MessageType::msg_type, handler);\
}

namespace warped {

class TimeWarpCommunicationManager {
public:
    virtual unsigned int initialize() = 0;

    virtual void finalize() = 0;

    virtual unsigned int getNumProcesses() = 0;

    virtual unsigned int getID() = 0;

    virtual int waitForAllProcesses() = 0;

    virtual int sumReduceUint64(uint64_t* send_local, uint64_t* recv_global) = 0;

    virtual int gatherUint64(uint64_t* send_local, uint64_t* recv_root) = 0;

    virtual int sumAllReduceInt64(int64_t* send_local, int64_t* recv_global) = 0;

    virtual int minAllReduceUint(unsigned int* send_local, unsigned int* recv_global) = 0;

    virtual void insertMessage(std::unique_ptr<TimeWarpKernelMessage> msg) = 0;

    // Sends all messages inserted into queue
    virtual void handleMessages() = 0;

    virtual void flushMessages() = 0;

    // Adds a MessageType/Message handler pair for dispatching messages
    void addRecvMessageHandler(MessageType msg_type,
        std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler);

    void initializeLPMap(const std::vector<std::vector<LogicalProcess*>>& lps);

    unsigned int getNodeID(std::string lp_name);

    virtual bool barrierHoldStatus() = 0;

    virtual void barrierPause() = 0;

    virtual void barrierResume() = 0;

    virtual bool getTokenSendConfirmation() = 0;

    virtual void setTokenSendConfirmation(bool input) = 0;

protected:
    // Map to lookup message handler given a message type
    std::unordered_map<int, std::function<void(std::unique_ptr<TimeWarpKernelMessage>)>>
        msg_handler_by_msg_type_;

private:
    std::unordered_map<std::string, unsigned int> node_id_by_lp_name_;

    bool sent_GVT_trigger_ = false;
};

} // namespace warped

#endif
