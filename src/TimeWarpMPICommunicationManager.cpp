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

    send_queue_ = std::make_shared<MPISendQueue>(send_queue_size_, max_buffer_size_);
    send_queue_->initialize();

    recv_queue_ = std::make_shared<MPIRecvQueue>(recv_queue_size_, max_buffer_size_);
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
    send_queue_->msg_list_lock_.lock();

    if (send_queue_->next_msg_pos_ > send_queue_->max_queue_size_) {
        std::cout << "Send queue too small: " << send_queue_->next_msg_pos_ << std::endl;
        abort();
    }
    send_queue_->msg_list_[send_queue_->next_msg_pos_++] = std::move(msg);

    send_queue_->msg_list_lock_.unlock();
}

void TimeWarpMPICommunicationManager::sendMessages() {

    // Get pending recvs
    testQueue(recv_queue_);

    // Complete pending sends
    send_queue_->msg_list_lock_.lock();
    testQueue(send_queue_);
    send_queue_->msg_list_lock_.unlock();

    // Start recvs
    recv_queue_->startRequests();

    // Start more sends
    send_queue_->startRequests();
}

std::unique_ptr<TimeWarpKernelMessage> TimeWarpMPICommunicationManager::getMessage() {
    // If empty
    if (consumer_pos_ == recv_queue_->next_msg_pos_) {
        return nullptr;
    }

    assert(recv_queue_->msg_list_[consumer_pos_]);

    auto msg = std::move(recv_queue_->msg_list_[consumer_pos_]);
    recv_queue_->msg_list_[consumer_pos_].reset(nullptr);

    consumer_pos_ = (consumer_pos_ + 1) % recv_queue_->max_queue_size_;

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
        buffer_list_[i] = make_unique<uint8_t []>(max_buffer_size_);
        std::memset(buffer_list_[i].get(), 0, max_buffer_size_*sizeof(uint8_t));
    }

    std::memset(request_list_.get(), 0, max_queue_size_*sizeof(MPI_Request));
    std::memset(index_list_.get(), 0, max_queue_size_*sizeof(int));
    std::memset(status_list_.get(), 0, max_queue_size_*sizeof(MPI_Status));
    std::memset(flag_list_.get(), 0, max_queue_size_*sizeof(int));
}

unsigned int MPISendQueue::startRequests() {
    int length = 0;
    unsigned int curr_msg_pos = 0;
    unsigned int requests = 0;

    msg_list_lock_.lock();

    while ((curr_msg_pos < next_msg_pos_) && (next_buffer_pos_ < max_queue_size_)) {

        assert(msg_list_[curr_msg_pos]);

        if (msg_list_[curr_msg_pos]->get_type() == MessageType::EventMessage) {
            auto event_msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(msg_list_[curr_msg_pos]));

            MatternNodeState::lock_.lock();
            if (MatternNodeState::color_ == MatternColor::WHITE) {
                MatternNodeState::white_msg_counter_++;
            } else {
                MatternNodeState::min_red_msg_timestamp_ =
                    std::min(MatternNodeState::min_red_msg_timestamp_, event_msg->event->timestamp());
            }
            MatternNodeState::lock_.unlock();

            msg_list_[curr_msg_pos] = std::move(event_msg);
        }

        // Serialize message
        std::ostringstream oss;
        {
            cereal::PortableBinaryOutputArchive oarchive(oss);
            oarchive(msg_list_[curr_msg_pos]);
        }

        // Copy serialized message to buffer
        length = oss.str().length();
        std::memcpy(buffer_list_[next_buffer_pos_].get(), oss.str().c_str(), length*sizeof(uint8_t));

        assert(msg_list_[curr_msg_pos] != nullptr);

        // Send message
        if (MPI_Isend(
                buffer_list_[next_buffer_pos_].get(),
                length,
                MPI_BYTE,
                msg_list_[curr_msg_pos]->receiver_id,
                MPI_DATA_TAG,
                MPI_COMM_WORLD,
                &request_list_[next_buffer_pos_]) != MPI_SUCCESS) {

            next_msg_pos_ -= requests;
            msg_list_lock_.unlock();
            return requests;
        }

        requests++;
        next_buffer_pos_++;

        msg_list_[curr_msg_pos].reset(nullptr);
        curr_msg_pos++;
    }

    assert(next_msg_pos_ >= requests);

    next_msg_pos_ -= requests;

    msg_list_lock_.unlock();

    return requests;
}

unsigned int MPIRecvQueue::startRequests() {
    int flag = 0;
    MPI_Status status;
    unsigned int requests = 0;

    while (next_buffer_pos_ < max_queue_size_) {

        MPI_Iprobe(
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
            MPI_COMM_WORLD,
            &flag,
            &status);

        if (flag) {

            assert(buffer_list_[next_buffer_pos_][0] == '\0');

            if (MPI_Irecv(
                    buffer_list_[next_buffer_pos_].get(),
                    max_buffer_size_,
                    MPI_BYTE,
                    MPI_ANY_SOURCE,
                    MPI_DATA_TAG,
                    MPI_COMM_WORLD,
                    &request_list_[next_buffer_pos_]) != MPI_SUCCESS) {

                return requests;
            }

            next_buffer_pos_++;

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

    if (!msg_queue->next_buffer_pos_)
        return 0;

    if (MPI_Testsome(
            msg_queue->next_buffer_pos_,
            msg_queue->request_list_.get(),
            &ready,
            msg_queue->index_list_.get(),
            msg_queue->status_list_.get()) != MPI_SUCCESS) {
        return 0;
    }

    if (ready < 1)
        return 0;

    // Complete pending sends/recvs
    for (int i = 0; i < ready; i++) {
        n = msg_queue->index_list_[i];

        msg_queue->completeRequest(msg_queue->buffer_list_[n].get());
    }

    // Collapse arrays
    for (unsigned int i = 0, n = 0; i < msg_queue->next_buffer_pos_; i++) {
        if (msg_queue->buffer_list_[i][0]) {
            if (i != n) {

                // Buffers
                msg_queue->buffer_list_[n].swap(msg_queue->buffer_list_[i]);

                // Requests
                std::memcpy(&msg_queue->request_list_[n],
                            &msg_queue->request_list_[i],
                            sizeof(MPI_Request));
            }
            n++;
        }
    }

    assert(msg_queue->next_buffer_pos_ >= (unsigned int)ready);

    msg_queue->next_buffer_pos_ -= ready;

    return ready;
}

void MPIRecvQueue::completeRequest(uint8_t *buffer) {

    // Deserialize message
    std::istringstream iss(std::string(reinterpret_cast<char*>(buffer), max_buffer_size_));
    {
        cereal::PortableBinaryInputArchive iarchive(iss);
        iarchive(msg_list_[next_msg_pos_]);
    }

    if (msg_list_[next_msg_pos_]->get_type() == MessageType::EventMessage) {
        auto event_msg = unique_cast<TimeWarpKernelMessage, EventMessage>(std::move(msg_list_[next_msg_pos_]));

        MatternNodeState::lock_.lock();
        if (event_msg->gvt_mattern_color == MatternColor::WHITE) {
            MatternNodeState::white_msg_counter_--;
        }
        MatternNodeState::lock_.unlock();

        msg_list_[next_msg_pos_] = std::move(event_msg);
    }

    next_msg_pos_ = (next_msg_pos_ + 1) % max_queue_size_;

    if (next_msg_pos_ == max_queue_size_) {
        std::cout << "Recv queue too small: " << max_queue_size_ << std::endl;
        abort();
    }

    std::memset(buffer, 0, max_buffer_size_*sizeof(uint8_t));
}

void MPISendQueue::completeRequest(uint8_t *buffer) {
    std::memset(buffer, 0, max_buffer_size_*sizeof(uint8_t));
}

} // namespace warped


