#include <cstring> // for memcpy
#include <algorithm> // for std::remove_if
#include <cstdint>  // for uint8_t type
#include <cassert>

#include "TimeWarpMPICommunicationManager.hpp"
#include "utility/memory.hpp"

namespace warped {

unsigned int TimeWarpMPICommunicationManager::initialize() {
    // MPI_Init requires command line arguments, but doesn't use them. Just give
    // it an empty vector.
    int argc = 0;
    char** argv = new char*[1];
    argv[0] = NULL;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    delete [] argv;

    if (provided == MPI_THREAD_SINGLE) {
        std::cout << "Warning: Your MPI Implementation only supports single-threaded applications" << std::endl;
    }

    return getNumProcesses();
}

void TimeWarpMPICommunicationManager::finalize() {
    assert(isInitiatingThread());
    MPI_Finalize();
}

void TimeWarpMPICommunicationManager::sendMessage(std::unique_ptr<TimeWarpKernelMessage> msg) {

    // Test all pending sends and mark the flag if complete
    for (auto const& m: pending_sends_) {
        MPI_Test(m->request.get(), &m->flag, m->status);
    }

    // Remove pending sends if flag is set
    pending_sends_.erase(std::remove_if(pending_sends_.begin(), pending_sends_.end(),
                                        [](std::unique_ptr<MPIMessage> &m){ return m->flag != 0; }),
                         pending_sends_.end());

    std::ostringstream oss;
    {
        cereal::PortableBinaryOutputArchive oarchive(oss);
        oarchive(msg);
    }

    int size = oss.str().size();
    std::unique_ptr<uint8_t[]> buf = make_unique<uint8_t[]>(size);
    memcpy(buf.get(), oss.str().c_str(), size);

    std::unique_ptr<MPI_Request> request = make_unique<MPI_Request>();

    assert(isInitiatingThread());
    MPI_Isend(buf.get(), size, MPI_BYTE, msg->receiver_id,
              MPI_DATA_TAG, MPI_COMM_WORLD, request.get());

    // Save pending send messages so buffer memory doesn't get deleted
    pending_sends_.push_back(make_unique<MPIMessage>(std::move(buf), std::move(request)));
}

std::unique_ptr<TimeWarpKernelMessage> TimeWarpMPICommunicationManager::recvMessage() {
    int flag = 0;
    MPI_Status status;
    uint8_t* message = nullptr;
    std::unique_ptr<TimeWarpKernelMessage> msg;

    assert(isInitiatingThread());
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

    if (flag != 0) {
        int size = 512;
        message = new uint8_t[size];

        MPI_Recv(message, size, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 &status);

        std::istringstream iss(std::string(reinterpret_cast<char*>(message), size));
        {
            cereal::PortableBinaryInputArchive iarchive(iss);
            iarchive(msg);
        }
        delete [] message;
    }
    return std::move(msg);
}

unsigned int TimeWarpMPICommunicationManager::getNumProcesses() {
    int size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    return size;
}

unsigned int TimeWarpMPICommunicationManager::getID() {
    int id = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    return id;
}

int TimeWarpMPICommunicationManager::waitForAllProcesses() {
    assert(isInitiatingThread());
    return MPI_Barrier(MPI_COMM_WORLD);
}

bool TimeWarpMPICommunicationManager::isInitiatingThread() {
    int flag = 0;
    MPI_Is_thread_main(&flag);
    return flag;
}

int TimeWarpMPICommunicationManager::sumReduceUint(unsigned int *send_local,
        unsigned int *recv_global) {
    assert(isInitiatingThread());
    return MPI_Reduce(send_local, recv_global, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
}

int TimeWarpMPICommunicationManager::gatherUint(unsigned int *send_local, unsigned int* recv_root) {
    assert(isInitiatingThread());
    return MPI_Gather(send_local, 1, MPI_UNSIGNED, recv_root, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
}

} // namespace warped


