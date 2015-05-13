#include <cstring> // for memcpy
#include <algorithm> // for std::remove_if
#include <cstdint>  // for uint8_t type
#include <cassert>

#include "TimeWarpMPICommunicationManager.hpp"
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

    send_queue_ = std::make_shared<MPISendQueue>(send_queue_size_);
    send_queue_->initialize();

    recv_queue_ = std::make_shared<MPIRecvQueue>(recv_queue_size_);
    recv_queue_->initialize();

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
TimeWarpMPICommunicationManager::gatherUint(unsigned int *send_local, unsigned int* recv_root) {
    assert(isInitiatingThread());
    return MPI_Gather(send_local, 1, MPI_UNSIGNED, recv_root, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
}

void TimeWarpMPICommunicationManager::insertMessage(std::unique_ptr<TimeWarpKernelMessage> msg) {
    send_queue_->msg_list_[send_queue_->next_msg_pos_] = std::move(msg);
    send_queue_->next_msg_pos_++;
}

void TimeWarpMPICommunicationManager::sendMessages() {
    // Complete pending sends
    testQueue(send_queue_);

    // Get pending recvs
    testQueue(recv_queue_);

    // Start more sends
    send_queue_->startRequest();
}

std::unique_ptr<TimeWarpKernelMessage> TimeWarpMPICommunicationManager::getMessage() {

    // If empty
    if (recv_queue_->next_msg_pos_ == 0) {
        return nullptr;
    }

    recv_queue_->next_msg_pos_--;

    auto msg = std::move(recv_queue_->msg_list_[recv_queue_->next_msg_pos_]);
    recv_queue_->msg_list_[recv_queue_->next_msg_pos_].reset(nullptr);

    return std::move(msg);
}

void MessageQueue::initialize() {
    msg_list_   = make_unique<std::unique_ptr<TimeWarpKernelMessage> []>(max_queue_size_);
    buffer_list_    = make_unique<std::unique_ptr<uint8_t []> []>(max_queue_size_);
    request_list_   = make_unique<MPI_Request []>(max_queue_size_);
    index_list_     = make_unique<int []>(max_queue_size_);
    status_list_    = make_unique<MPI_Status []>(max_queue_size_);
    flag_list_      = make_unique<int []>(max_queue_size_);

    for (unsigned int i = 0; i < max_queue_size_; i++) {
        msg_list_[i] = nullptr;
        buffer_list_[i] = make_unique<uint8_t []>(MAX_BUFFER_SIZE);
    }

    std::memset(request_list_.get(), 0, max_queue_size_*sizeof(MPI_Request));
    std::memset(index_list_.get(), 0, max_queue_size_*sizeof(int));
    std::memset(status_list_.get(), 0, max_queue_size_*sizeof(MPI_Status));
    std::memset(flag_list_.get(), 0, max_queue_size_*sizeof(int));
}

void MPISendQueue::startRequest() {
    int length = 0;

    for (unsigned int i = 0; i < next_msg_pos_; i++) {

        // Serialize message
        std::ostringstream oss;
        {
            cereal::PortableBinaryOutputArchive oarchive(oss);
            oarchive(msg_list_[i]);
        }

        // Copy serialized message to buffer
        length = oss.str().length()+1;
        std::memcpy(buffer_list_[i].get(), oss.str().c_str(), length*sizeof(uint8_t));

        // Send message
        MPI_Isend(
            buffer_list_[i].get(),
            length,
            MPI_BYTE,
            msg_list_[i]->receiver_id,
            MPI_DATA_TAG,
            MPI_COMM_WORLD,
            &request_list_[i]);
    }
}

void MPIRecvQueue::startRequest() {
    int flag = 0;
    MPI_Status status;
    int count = 0;

    MPI_Iprobe(
        MPI_ANY_SOURCE,
        MPI_ANY_TAG,
        MPI_COMM_WORLD,
        &flag,
        &status);

    if (flag) {

        if (MPI_Get_count(&status, MPI_BYTE, &count) != MPI_SUCCESS) {
            return;
        }

        if (MPI_Irecv(
                buffer_list_[next_buffer_pos_].get(),
                count,
                MPI_BYTE,
                MPI_ANY_SOURCE,
                MPI_DATA_TAG,
                MPI_COMM_WORLD,
                &request_list_[next_buffer_pos_]) != MPI_SUCCESS) {
            return;
        }

        next_buffer_pos_++;
    }
}

void TimeWarpMPICommunicationManager::testQueue(std::shared_ptr<MessageQueue> msg_queue) {
    int ready;
    unsigned int n;

    if (MPI_Testsome(
            msg_queue->next_buffer_pos_,
            msg_queue->request_list_.get(),
            &ready,
            msg_queue->index_list_.get(),
            msg_queue->status_list_.get()) != MPI_SUCCESS) {
        return;
    }

    if (!ready)
        return;

    // Complete pending sends/recvs
    for (int i = 0; i < ready; i++) {
        n = msg_queue->index_list_[i];

        auto buf = std::move(msg_queue->buffer_list_[n]);
        msg_queue->buffer_list_[n].reset(new uint8_t[MAX_BUFFER_SIZE]);

        msg_queue->completeRequest(std::move(buf), n);
    }

    // Collapse arrays
    for (unsigned int i = 0, n = 0; i < msg_queue->next_buffer_pos_; i++) {
        if (msg_queue->msg_list_[i]) {
            if (i != n) {
                // Messages
                msg_queue->msg_list_[n] = std::move(msg_queue->msg_list_[i]);

                // Requests
                std::memcpy(&msg_queue->request_list_[n], &msg_queue->request_list_[i],
                            sizeof(MPI_Request));

                // Buffers
                msg_queue->buffer_list_[n].swap(msg_queue->buffer_list_[i]);

                // Index, status, and flags are only MPI output parameters and can be ignored.
            }
            n++;
        }
    }

    msg_queue->next_buffer_pos_ -= ready;
}

void MPIRecvQueue::completeRequest(std::unique_ptr<uint8_t []> buffer, unsigned int index) {
    unused(index);

    // Deserialize message
    unsigned int count = strlen(reinterpret_cast<char*>(buffer.get()));
    std::istringstream iss(std::string(reinterpret_cast<char*>(buffer.get()), count));
    {
        cereal::PortableBinaryInputArchive iarchive(iss);
        iarchive(msg_list_[next_msg_pos_]);
    }

    next_msg_pos_++;
}

void MPISendQueue::completeRequest(std::unique_ptr<uint8_t []> buffer, unsigned int index) {
    unused(std::move(buffer));

    // Free msg
    msg_list_[index].reset(nullptr);
}

} // namespace warped


