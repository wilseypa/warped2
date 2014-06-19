#ifndef MPI_COMMUNICATION_MANAGER_HPP
#define MPI_COMMUNICATION_MANAGER_HPP

#include <mpi.h>
#include <mutex>
#include <deque>
#include <vector>

#include "KernelMessage.hpp"

namespace warped {

#define MPI_DATA_TAG  100

struct MPIMessage {
    MPIMessage(std::unique_ptr<char> m, std::unique_ptr<MPI_Request> r) :
        msg(std::move(m)),
        request(std::move(r)) {}

    std::unique_ptr<char> msg;
    std::unique_ptr<MPI_Request> request;
    MPI_Status *status = MPI_STATUS_IGNORE;
    int flag = 0;
};

class MPICommunicationManager {
public:
    void initialize();
    void finalize();
    unsigned int getNumProcesses();
    unsigned int getID();

    void sendMessage(std::unique_ptr<KernelMessage> msg);
    void sendAllMessages();

    void enqueueMessage(std::unique_ptr<KernelMessage> msg);

    std::unique_ptr<KernelMessage> recvMessage();

protected:
    std::unique_ptr<KernelMessage> dequeueMessage();

private:
    std::mutex send_queue_mutex_;
    std::deque<std::unique_ptr<KernelMessage>> send_queue_;

    // Holds pending messages, this is needed so message buffer isn't deleted before the message is
    // sent and we dont have to wait for for sends.
    std::vector<std::unique_ptr<MPIMessage>> pending_sends_;
};

} // namespace warped

#endif

