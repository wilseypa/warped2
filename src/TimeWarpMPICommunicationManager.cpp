#include <cstring> // for memcpy
#include <algorithm> // for std::remove_if
#include <cstdint>  // for uint8_t type
#include <cassert>

#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpEventDispatcher.hpp"          // for EventMessage
#include "TimeWarpMatternGVTManager.hpp"
#include "utility/memory.hpp"
#include "utility/warnings.hpp"

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
        std::cout << "Warning: Your MPI Implementation only supports single-threaded applications"
                  << std::endl;
    }

    send_queue_ = std::make_shared<MPISendQueue>(max_buffer_size_);
    recv_queue_ = std::make_shared<MPIRecvQueue>(max_buffer_size_);

    return getNumProcesses();
}

void TimeWarpMPICommunicationManager::finalize() {
    assert(isInitiatingThread());
    MPI_Finalize();
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

int
TimeWarpMPICommunicationManager::sumReduceUint64(uint64_t *send_local, uint64_t *recv_global) {
    assert(isInitiatingThread());
    return MPI_Reduce(send_local, recv_global, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
}

int
TimeWarpMPICommunicationManager::gatherUint64(uint64_t *send_local, uint64_t *recv_root) {
    assert(isInitiatingThread());
    return MPI_Gather(send_local, 1, MPI_UINT64_T, recv_root, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
}

void TimeWarpMPICommunicationManager::insertMessage(std::unique_ptr<TimeWarpKernelMessage> msg) {
    send_queue_->msg_list_lock_.lock();
    send_queue_->msg_list_.push_back(std::move(msg));
    send_queue_->msg_list_lock_.unlock();
}

void TimeWarpMPICommunicationManager::sendMessages() {

    // Get pending recvs
    testQueue(recv_queue_);

    // Complete pending sends
    testQueue(send_queue_);

    // Start recvs
    recv_queue_->startRequests();

    // Start more sends
    send_queue_->startRequests();
}

std::unique_ptr<TimeWarpKernelMessage> TimeWarpMPICommunicationManager::getMessage() {
    if (recv_queue_->msg_list_.empty()) {
        return nullptr;
    }
    auto msg = std::move(recv_queue_->msg_list_.front());
    recv_queue_->msg_list_.pop_front();

    return std::move(msg);
}

unsigned int MPISendQueue::startRequests() {
    int length = 0;
    unsigned int requests = 0;

    msg_list_lock_.lock();

    while (!msg_list_.empty()) {

        auto msg = std::move(msg_list_.front());
        msg_list_.pop_front();

        // Serialize message
        std::ostringstream oss;
        {
            cereal::PortableBinaryOutputArchive oarchive(oss);
            oarchive(msg);
        }

        // Copy serialized message to buffer
        length = oss.str().length();
        buffer_list_.emplace_back(new uint8_t[max_buffer_size_]);
        std::memcpy(buffer_list_.back().get(), oss.str().c_str(), length*sizeof(uint8_t));

        request_list_.emplace_back();

        // Send message
        if (MPI_Isend(
                buffer_list_.back().get(),
                length,
                MPI_BYTE,
                msg->receiver_id,
                MPI_DATA_TAG,
                MPI_COMM_WORLD,
                &request_list_.back()) != MPI_SUCCESS) {

            msg_list_lock_.unlock();
            return requests;
        }

        requests++;
    }

    msg_list_lock_.unlock();

    return requests;
}

unsigned int MPIRecvQueue::startRequests() {
    int flag = 0;
    MPI_Status status;
    unsigned int requests = 0;

    while (true) {

        MPI_Iprobe(
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &flag,
            &status);

        if (flag) {
            buffer_list_.emplace_back(new uint8_t[max_buffer_size_]);
            request_list_.emplace_back();

            if (MPI_Irecv(
                    buffer_list_.back().get(),
                    max_buffer_size_,
                    MPI_BYTE,
                    MPI_ANY_SOURCE,
                    MPI_DATA_TAG,
                    MPI_COMM_WORLD,
                    &request_list_.back()) != MPI_SUCCESS) {

                return requests;
            }
        } else {
            break;
        }

        requests++;
    }

    return requests;
}

int TimeWarpMPICommunicationManager::testQueue(std::shared_ptr<MessageQueue> msg_queue) {
    int ready = 0;
    unsigned int n;

    if (msg_queue->buffer_list_.empty())
        return 0;

    std::vector<int> index_list(msg_queue->buffer_list_.size());

    if (MPI_Testsome(
            msg_queue->buffer_list_.size(),
            &msg_queue->request_list_[0],
            &ready,
            &index_list[0],
            MPI_STATUSES_IGNORE) != MPI_SUCCESS) {
        return 0;
    }

    if (ready < 1)
        return 0;

    // Complete pending sends/recvs
    for (int i = 0; i < ready; i++) {
        n = index_list[i];
        msg_queue->completeRequest(msg_queue->buffer_list_[n].get());

    }

    for (int i = (ready-1); i >= 0; i--) {
        n = index_list[i];
        msg_queue->buffer_list_.erase(msg_queue->buffer_list_.begin() + n);
        msg_queue->request_list_.erase(msg_queue->request_list_.begin() + n);
    }

    return ready;
}

void MPIRecvQueue::completeRequest(uint8_t *buffer) {

    std::unique_ptr<TimeWarpKernelMessage> msg = nullptr;

    // Deserialize message
    std::istringstream iss(std::string(reinterpret_cast<char*>(buffer), max_buffer_size_));
    {
        cereal::PortableBinaryInputArchive iarchive(iss);
        iarchive(msg);
    }

    msg_list_.push_back(std::move(msg));
}

void MPISendQueue::completeRequest(uint8_t *buffer) {
    unused(buffer);
}

} // namespace warped


