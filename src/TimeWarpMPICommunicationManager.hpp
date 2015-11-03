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
    void handleMessages();

    unsigned int startSendRequests();
    unsigned int startReceiveRequests();
    unsigned int testSendRequests();
    unsigned int testReceiveRequests();

    bool isInitiatingThread();

    int sumReduceUint64(uint64_t* send_local, uint64_t* recv_global);
    int gatherUint64(uint64_t* send_local, uint64_t* recv_root);

private:
    unsigned int max_buffer_size_;

    int num_processes_;
    int my_rank_;

    std::shared_ptr<MessageQueue> send_queue_;
    std::shared_ptr<MessageQueue> recv_queue_;
};

struct PendingRequest {
    PendingRequest(std::unique_ptr<uint8_t[]> buffer) : buffer_(std::move(buffer)) {}

    std::unique_ptr<uint8_t[]> buffer_;
    MPI_Request request_;
    int flag_;
    MPI_Status status_;
};

struct MessageQueue {
    MessageQueue(unsigned int max_buffer_size) :
        max_buffer_size_(max_buffer_size) {}

    unsigned int max_buffer_size_;

    std::deque<std::unique_ptr<TimeWarpKernelMessage>>  msg_list_;
    TicketLock msg_list_lock_;

    std::vector<std::unique_ptr<PendingRequest>> pending_request_list_;
};

} // namespace warped

#endif

