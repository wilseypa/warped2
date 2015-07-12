#ifndef MPI_COMMUNICATION_MANAGER_HPP
#define MPI_COMMUNICATION_MANAGER_HPP

#include <mpi.h>
#include <vector>
#include <cstdint>
#include <mutex>

#include "TimeWarpCommunicationManager.hpp"
#include "TimeWarpKernelMessage.hpp"
#include "TicketLock.hpp"

namespace warped {

struct MessageQueue;
struct MPISendQueue;
struct MPIRecvQueue;

#define MPI_DATA_TAG    729

class TimeWarpMPICommunicationManager : public TimeWarpCommunicationManager {
public:
    TimeWarpMPICommunicationManager(unsigned int max_buffer_size) :
        max_buffer_size_(max_buffer_size) {}

    unsigned int initialize();
    void finalize();
    unsigned int getNumProcesses();
    unsigned int getID();
    int waitForAllProcesses();

    void insertMessage(std::unique_ptr<TimeWarpKernelMessage> msg);
    void sendMessages();
    std::unique_ptr<TimeWarpKernelMessage> getMessage();

    bool isInitiatingThread();

    int sumReduceUint64(uint64_t* send_local, uint64_t* recv_global);
    int gatherUint64(uint64_t* send_local, uint64_t* recv_root);

private:
    int testQueue(std::shared_ptr<MessageQueue> msg_queue);

    unsigned int max_buffer_size_;

    std::shared_ptr<MPISendQueue> send_queue_;
    std::shared_ptr<MPIRecvQueue> recv_queue_;
};

struct MessageQueue {
    MessageQueue(unsigned int max_buffer_size) :
        max_buffer_size_(max_buffer_size) {}

    virtual unsigned int startRequests() = 0;
    virtual void completeRequest(uint8_t *buffer) = 0;

    unsigned int max_buffer_size_;

    std::deque<std::unique_ptr<TimeWarpKernelMessage>>  msg_list_;
    TicketLock msg_list_lock_;

    std::vector<std::unique_ptr<uint8_t []>>             buffer_list_;
    std::vector<MPI_Request>                             request_list_;
};

struct MPISendQueue : public MessageQueue {
    MPISendQueue(unsigned int max_buffer_size) : MessageQueue(max_buffer_size) {}
    unsigned int startRequests();
    void completeRequest(uint8_t *buffer);
};

struct MPIRecvQueue : public MessageQueue {
    MPIRecvQueue(unsigned int max_buffer_size) : MessageQueue(max_buffer_size) {}
    unsigned int startRequests();
    void completeRequest(uint8_t *buffer);
};

} // namespace warped

#endif

