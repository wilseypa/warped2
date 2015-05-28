#ifndef MPI_COMMUNICATION_MANAGER_HPP
#define MPI_COMMUNICATION_MANAGER_HPP

#include <mpi.h>
#include <vector>
#include <cstdint>
#include <mutex>

#include "TimeWarpCommunicationManager.hpp"
#include "TimeWarpKernelMessage.hpp"

namespace warped {

struct MessageQueue;
struct MPISendQueue;
struct MPIRecvQueue;

#define MPI_DATA_TAG    729

class TimeWarpMPICommunicationManager : public TimeWarpCommunicationManager {
public:
    TimeWarpMPICommunicationManager(unsigned int send_queue_size, unsigned int recv_queue_size,
        unsigned int max_buffer_size) :
    send_queue_size_(send_queue_size), recv_queue_size_(recv_queue_size),
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
    int gatherUint(unsigned int *send_local, unsigned int* recv_root);

private:
    int testQueue(std::shared_ptr<MessageQueue> msg_queue);

    unsigned int send_queue_size_;
    unsigned int recv_queue_size_;
    unsigned int max_buffer_size_;

    unsigned int consumer_pos_ = 0;

    std::shared_ptr<MPISendQueue> send_queue_;
    std::shared_ptr<MPIRecvQueue> recv_queue_;
};

struct MessageQueue {
    MessageQueue(unsigned int max_queue_size, unsigned int max_buffer_size) :
        max_queue_size_(max_queue_size), max_buffer_size_(max_buffer_size) {}

    void initialize();

    virtual unsigned int startRequests() = 0;
    virtual void completeRequest(uint8_t *buffer) = 0;

    unsigned int max_queue_size_;
    unsigned int max_buffer_size_;

    // serialized buffer position
    unsigned int next_buffer_pos_ = 0;

    // deserialized msg position
    unsigned int next_msg_pos_ = 0;

    std::unique_ptr<std::unique_ptr<TimeWarpKernelMessage> []>  msg_list_;
    std::mutex msg_list_lock_;

    std::unique_ptr<std::unique_ptr<uint8_t []> []>             buffer_list_;
    std::unique_ptr<MPI_Request []>                             request_list_;
    std::unique_ptr<int []>                                     index_list_;
    std::unique_ptr<MPI_Status []>                              status_list_;
    std::unique_ptr<int []>                                     flag_list_;
};

struct MPISendQueue : public MessageQueue {
    MPISendQueue(unsigned int send_queue_size, unsigned int max_buffer_size) :
        MessageQueue(send_queue_size, max_buffer_size) {}
    unsigned int startRequests();
    void completeRequest(uint8_t *buffer);
};

struct MPIRecvQueue : public MessageQueue {
    MPIRecvQueue(unsigned int recv_queue_size, unsigned int max_buffer_size) :
        MessageQueue(recv_queue_size, max_buffer_size) {}
    unsigned int startRequests();
    void completeRequest(uint8_t *buffer);
};

} // namespace warped

#endif

