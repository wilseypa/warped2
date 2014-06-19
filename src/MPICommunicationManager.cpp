#include "MPICommunicationManager.hpp"
#include "utility/memory.hpp"

#include <cstring>  // for std::strcpy
#include <algorithm> // for std::remove_if

namespace warped {

void MPICommunicationManager::initialize() {
    // MPI_Init requires command line arguments, but doesn't use them. Just give
    // it an empty vector.
    int argc = 0;
    char** argv = new char*[1];
    argv[0] = NULL;

    MPI_Init(&argc, &argv);

    delete [] argv;
}

void MPICommunicationManager::finalize() {
    MPI_Finalize();
}

// TODO ALL OF THIS NEEDS TO BE TESTED
void MPICommunicationManager::sendMessage(std::unique_ptr<KernelMessage> msg) {

    // Test all pending sends and mark the flag if complete
    for (auto const& m: pending_sends_) {
        MPI_Test(m->request.get(), &m->flag, m->status);
    }

    // TODO this needs to be tested
    pending_sends_.erase(std::remove_if(pending_sends_.begin(), pending_sends_.end(),
                                        [](std::unique_ptr<MPIMessage> &m){ return m->flag != 0; }),
                         pending_sends_.end());

    std::stringstream ss;
    cereal::BinaryOutputArchive oarchive(ss);
    oarchive(msg);

    std::unique_ptr<char> buf = make_unique<char>();
    std::strcpy(buf.get(), ss.str().c_str());
    std::unique_ptr<MPI_Request> request;

    MPI_Isend(const_cast<char*> (buf.get()), ss.str().size(), MPI_BYTE, msg->receiver_id,
              MPI_DATA_TAG, MPI_COMM_WORLD, request.get());

    // Save pending send messages so buffer memory doesn't get deleted
    pending_sends_.push_back(make_unique<MPIMessage>(std::move(buf), std::move(request)));
}

void MPICommunicationManager::sendAllMessages() {
    auto next = dequeueMessage();
    while (next) {
        sendMessage(std::move(next));
        next = dequeueMessage();
    }
}

// TODO I have no idea if this will actually work, I will probably have to mess with this
// a little bit and test it pretty thoroughly.
std::unique_ptr<KernelMessage> MPICommunicationManager::recvMessage() {
    int flag = 0;
    MPI_Status status;
    int count = 0;
    char* message = nullptr;
    std::stringstream ss;
    std::unique_ptr<KernelMessage> msg = nullptr;

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);

    if (flag != 0) {
        MPI_Get_count(&status, MPI_BYTE, &count);
        message = new char[count];
        MPI_Recv(message, count, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        ss << message;
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(msg);
    }
    return msg;
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

void MPICommunicationManager::enqueueMessage(std::unique_ptr<KernelMessage> msg) {
    send_queue_mutex_.lock();
    send_queue_.push_back(std::move(msg));
    send_queue_mutex_.unlock();
}

std::unique_ptr<KernelMessage> MPICommunicationManager::dequeueMessage() {
    std::unique_ptr<KernelMessage> msg = nullptr;
    send_queue_mutex_.lock();
    if (!send_queue_.empty()) {
        msg = std::move(send_queue_.front());
        send_queue_.pop_front();
    }
    send_queue_mutex_.unlock();
    return msg;
}

} // namespace warped

