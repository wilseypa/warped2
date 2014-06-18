#include "MPICommunicationManager.hpp"

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

void MPICommunicationManager::sendMessage(std::unique_ptr<KernelMessage> msg) {
    std::stringstream ss;
    cereal::BinaryOutputArchive oarchive(ss);

    oarchive(msg);
    const char* buf = ss.str().c_str();
    MPI_Request request;
    MPI_Isend(const_cast<char*> (&buf[0]), ss.str().size(), MPI_BYTE, msg->receiverID,
              MPI_DATA_TAG, MPI_COMM_WORLD, &request);
}

void MPICommunicationManager::sendAllMessages() {
    auto next = dequeueMessage();
    while (next) {
        sendMessage(std::move(next));
        next = dequeueMessage();
    }
}

std::unique_ptr<KernelMessage> MPICommunicationManager::recvMessage() {
    return nullptr;// TODO
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

