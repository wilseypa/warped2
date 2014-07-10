#ifndef MPI_COMMUNICATION_MANAGER_HPP
#define MPI_COMMUNICATION_MANAGER_HPP

#include <mpi.h>
#include <vector>
#include <cstdint>

#include "TimeWarpCommunicationManager.hpp"
#include "TimeWarpKernelMessage.hpp"

namespace warped {

#define MPI_DATA_TAG  100

struct MPIMessage {
    MPIMessage(std::unique_ptr<uint8_t[]> m, std::unique_ptr<MPI_Request> r) :
        msg(std::move(m)),
        request(std::move(r)) {}

    std::unique_ptr<uint8_t[]> msg;
    std::unique_ptr<MPI_Request> request;
    MPI_Status *status = MPI_STATUS_IGNORE;
    int flag = 0;
};

class TimeWarpMPICommunicationManager : public TimeWarpCommunicationManager {
public:
    unsigned initialize();
    void finalize();
    unsigned int getNumProcesses();
    unsigned int getID();

    void sendMessage(std::unique_ptr<TimeWarpKernelMessage> msg);
    int sendAllMessages();
    std::unique_ptr<TimeWarpKernelMessage> recvMessage();

private:
    std::vector<std::unique_ptr<MPIMessage>> pending_sends_;
};

} // namespace warped

#endif

