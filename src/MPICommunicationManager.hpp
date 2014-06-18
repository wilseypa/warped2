#ifndef MPI_COMMUNICATION_MANAGER_HPP
#define MPI_COMMUNICATION_MANAGER_HPP

#include <mpi.h>
#include <mutex>
#include <deque>

#include "KernelMessage.hpp"

namespace warped {

#define MPI_DATA_TAG  100

class MPICommunicationManager {
public:
    void initialize();
    void finalize();
    unsigned int getNumProcesses();
    unsigned int getID();

    void sendMessage(std::unique_ptr<KernelMessage> msg);
    void sendAllMessages();

    void enqueueMessage(std::unique_ptr<KernelMessage> msg);

protected:
    std::unique_ptr<KernelMessage> dequeueMessage();
    std::unique_ptr<KernelMessage> recvMessage();

private:
    std::mutex send_queue_mutex_;
    std::deque<std::unique_ptr<KernelMessage>> send_queue_;
};

} // namespace warped

#endif

