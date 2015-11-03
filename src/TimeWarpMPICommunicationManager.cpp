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

    send_queue_ = std::make_shared<MessageQueue>(max_buffer_size_);
    recv_queue_ = std::make_shared<MessageQueue>(max_buffer_size_);

    MPI_Comm_size(MPI_COMM_WORLD, &num_processes_);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank_);

    return getNumProcesses();
}

void TimeWarpMPICommunicationManager::finalize() {
    assert(isInitiatingThread());
    MPI_Finalize();
}

unsigned int TimeWarpMPICommunicationManager::getNumProcesses() {
    return num_processes_;
}

unsigned int TimeWarpMPICommunicationManager::getID() {
    return my_rank_;
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

void TimeWarpMPICommunicationManager::handleMessages() {
    testReceiveRequests();
    testSendRequests();
    startReceiveRequests();
    startSendRequests();
}

unsigned int TimeWarpMPICommunicationManager::startSendRequests() {
    unsigned int requests = 0;

    send_queue_->msg_list_lock_.lock();

    while (!send_queue_->msg_list_.empty()) {

        auto msg = std::move(send_queue_->msg_list_.front());
        send_queue_->msg_list_.pop_front();

        unsigned int receiver_id = msg->receiver_id;

        std::ostringstream oss;
        {
            cereal::PortableBinaryOutputArchive oarchive(oss);
            oarchive(std::move(msg));
        }

        auto new_request = make_unique<PendingRequest>(make_unique<uint8_t[]>(max_buffer_size_));
        send_queue_->pending_request_list_.push_back(std::move(new_request));
        std::memcpy(send_queue_->pending_request_list_.back()->buffer_.get(), oss.str().c_str(),
                    oss.str().length()+1);

        if (MPI_Isend(
                send_queue_->pending_request_list_.back()->buffer_.get(),
                oss.str().length()+1,
                MPI_BYTE,
                receiver_id,
                MPI_DATA_TAG,
                MPI_COMM_WORLD,
                &send_queue_->pending_request_list_.back()->request_) != MPI_SUCCESS) {

            send_queue_->msg_list_lock_.unlock();
            return requests;
        }

        requests++;
    }

    send_queue_->msg_list_lock_.unlock();

    return requests;
}

unsigned int TimeWarpMPICommunicationManager::startReceiveRequests() {
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
            auto new_request = make_unique<PendingRequest>(make_unique<uint8_t[]>(max_buffer_size_));
            recv_queue_->pending_request_list_.push_back(std::move(new_request));

            if (MPI_Irecv(
                    recv_queue_->pending_request_list_.back()->buffer_.get(),
                    max_buffer_size_,
                    MPI_BYTE,
                    MPI_ANY_SOURCE,
                    MPI_DATA_TAG,
                    MPI_COMM_WORLD,
                    &recv_queue_->pending_request_list_.back()->request_) != MPI_SUCCESS) {

                return requests;
            }
        } else {
            break;
        }

        requests++;
    }

    return requests;
}

unsigned int TimeWarpMPICommunicationManager::testSendRequests() {
    unsigned int count = 0;

    for (auto &pr : send_queue_->pending_request_list_) {
        MPI_Test(&pr->request_, &pr->flag_, &pr->status_);
        if (pr->flag_) count++;
    }

    send_queue_->pending_request_list_.erase(
        std::remove_if(
            send_queue_->pending_request_list_.begin(),
            send_queue_->pending_request_list_.end(),
            [&](const std::unique_ptr<PendingRequest>& pr) { return pr->flag_; }),
        send_queue_->pending_request_list_.end());

    return count;
}

unsigned int TimeWarpMPICommunicationManager::testReceiveRequests() {
    unsigned int count = 0;

    for (auto &pr : recv_queue_->pending_request_list_) {
        MPI_Test(&pr->request_, &pr->flag_, &pr->status_);
        if (pr->flag_) {
            count++;

            std::unique_ptr<TimeWarpKernelMessage> msg = nullptr;
            std::istringstream iss(std::string(reinterpret_cast<char*>(pr->buffer_.get()), max_buffer_size_));
            cereal::PortableBinaryInputArchive iarchive(iss);
            iarchive(msg);

            MessageType msg_type = msg->get_type();
            int msg_type_int = static_cast<int>(msg_type);
            msg_handler_by_msg_type_[msg_type_int](std::move(msg));
        }
    }

    recv_queue_->pending_request_list_.erase(
        std::remove_if(
            recv_queue_->pending_request_list_.begin(),
            recv_queue_->pending_request_list_.end(),
            [&](const std::unique_ptr<PendingRequest>& pr) { return pr->flag_; }),
        recv_queue_->pending_request_list_.end());

    return count;

}

} // namespace warped


