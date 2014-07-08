#include <cstring> // for memcpy
#include <algorithm> // for std::remove_if
#include <cstdint>  // for uint8_t type

#include "MPICommunicationManager.hpp"
#include "utility/memory.hpp"

namespace warped {

unsigned int MPICommunicationManager::initialize() {
    // MPI_Init requires command line arguments, but doesn't use them. Just give
    // it an empty vector.
    int argc = 0;
    char** argv = new char*[1];
    argv[0] = NULL;

    MPI_Init(&argc, &argv);

    delete [] argv;

    return getNumProcesses();
}

void MPICommunicationManager::finalize() {
    MPI_Finalize();
}

void MPICommunicationManager::sendMessage(std::unique_ptr<KernelMessage> msg) {

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

    MPI_Isend(buf.get(), size, MPI_BYTE, msg->receiver_id,
              MPI_DATA_TAG, MPI_COMM_WORLD, request.get());

    // Save pending send messages so buffer memory doesn't get deleted
    pending_sends_.push_back(make_unique<MPIMessage>(std::move(buf), std::move(request)));
}

int MPICommunicationManager::sendAllMessages() {
    int count = 0;
    auto next = dequeueMessage();
    while (next) {
        sendMessage(std::move(next));
        count++;
        next = dequeueMessage();
    }
    return count;
}

std::unique_ptr<KernelMessage> MPICommunicationManager::recvMessage() {
    int flag = 0;
    MPI_Status status;
    int count = 0;
    uint8_t* message = nullptr;
    std::unique_ptr<KernelMessage> msg;

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

    if (flag != 0) {
        MPI_Get_count(&status, MPI_BYTE, &count);

        message = new uint8_t[count];

        MPI_Recv(message, count, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 &status);

        std::istringstream iss(std::string(reinterpret_cast<char*>(message), count));
        {
            cereal::PortableBinaryInputArchive iarchive(iss);
            iarchive(msg);
        }
        delete [] message;
    }
    return std::move(msg);
}

unsigned int MPICommunicationManager::getNumProcesses() {
    int size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    return size;
}

unsigned int MPICommunicationManager::getID() {
    int id = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    return id;
}

} // namespace warped

