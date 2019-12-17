#include <cstring> // for memcpy
#include <algorithm> // for std::remove_if
#include <cstdint>  // for uint8_t type
#include <cassert>

#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpEventDispatcher.hpp"          // for EventMessage
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

    barrier_hold_ = false;

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

    aggregate_message_count_by_receiver_ = make_unique<unsigned int[]>(num_processes_);
    std::memset(aggregate_message_count_by_receiver_.get(), 0, num_processes_*sizeof(unsigned int));

    aggregate_messages_ = make_unique<std::list<std::unique_ptr<TimeWarpKernelMessage>>[]>(num_processes_);

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

int TimeWarpMPICommunicationManager::sumAllReduceInt64(int64_t* send_local, int64_t* recv_global) {
    assert(isInitiatingThread());
    return MPI_Allreduce(send_local, recv_global, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);
}

int TimeWarpMPICommunicationManager::minAllReduceUint(unsigned int* send_local, unsigned int* recv_global) {
    assert(isInitiatingThread());
    return MPI_Allreduce(send_local, recv_global, 1, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD);
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

void TimeWarpMPICommunicationManager::flushMessages() {
    for (unsigned int receiver_id = 0; receiver_id < getNumProcesses(); receiver_id++) {
        packAndSend(receiver_id);
    }
}

unsigned int TimeWarpMPICommunicationManager::startSendRequests() {
    unsigned int requests = 0;

    send_queue_->msg_list_lock_.lock();

    while (!send_queue_->msg_list_.empty()) {

        auto msg = std::move(send_queue_->msg_list_.front());
        send_queue_->msg_list_.pop_front();

        unsigned int receiver_id = msg->receiver_id;
        auto msg_type = msg->get_type();

        aggregate_messages_[receiver_id].push_back(std::move(msg));

        if ((++aggregate_message_count_by_receiver_[receiver_id] >= max_aggregate_) ||
            (msg_type != MessageType::EventMessage)) {

            packAndSend(receiver_id);

            requests++;
        }
    }

    send_queue_->msg_list_lock_.unlock();

    return requests;
}

void TimeWarpMPICommunicationManager::packAndSend(unsigned int receiver_id) {

    int position = 0;
    uint8_t* message_buffer = new uint8_t[max_buffer_size_];
    int num_messages = aggregate_messages_[receiver_id].size();
    int header_size = sizeof(int)*(num_messages+1);
    int message_position = header_size;

    MPI_Pack(&num_messages, 1, MPI_INT, message_buffer, max_buffer_size_, &position, MPI_COMM_WORLD);
    for (auto& m : aggregate_messages_[receiver_id]) {
        std::ostringstream oss;
        cereal::PortableBinaryOutputArchive oarchive(oss);
        oarchive(m);

        int length = oss.str().length();
        MPI_Pack(&length, 1, MPI_INT, message_buffer, max_buffer_size_, &position, MPI_COMM_WORLD);
        MPI_Pack(const_cast<char*>(oss.str().c_str()), length, MPI_BYTE, message_buffer,
                 max_buffer_size_, &message_position, MPI_COMM_WORLD);
    }

    auto new_request = make_unique<PendingRequest>(std::unique_ptr<uint8_t[]>(message_buffer), message_position);
    send_queue_->pending_request_list_.push_back(std::move(new_request));

    if (MPI_Isend(
            send_queue_->pending_request_list_.back()->buffer_.get(),
            send_queue_->pending_request_list_.back()->count_,
            MPI_PACKED,
            receiver_id,
            MPI_DATA_TAG,
            MPI_COMM_WORLD,
            &send_queue_->pending_request_list_.back()->request_) != MPI_SUCCESS) {
        throw std::runtime_error("MPI_Isend failed");
    }

    aggregate_messages_[receiver_id].clear();
    aggregate_message_count_by_receiver_[receiver_id] = 0;
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

            auto new_request = make_unique<PendingRequest>(make_unique<uint8_t[]>(max_buffer_size_), max_buffer_size_);
            recv_queue_->pending_request_list_.push_back(std::move(new_request));

            if (MPI_Irecv(
                    recv_queue_->pending_request_list_.back()->buffer_.get(),
                    recv_queue_->pending_request_list_.back()->count_,
                    MPI_PACKED,
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
        if (MPI_Test(&pr->request_, &pr->flag_, &pr->status_) != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Test failed in testSendRequest");
        }
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
        if (MPI_Test(&pr->request_, &pr->flag_, &pr->status_) != MPI_SUCCESS) {
            throw std::runtime_error("MPI_Test failed in testSendRequest");
        }
        if (pr->flag_) {
            count++;

            int num_messages;
            char* message_buffer = reinterpret_cast<char*>(pr->buffer_.get());
            int message_length = pr->count_;;
            int position = 0;

            MPI_Unpack(message_buffer, message_length, &position, &num_messages, 1, MPI_INT, MPI_COMM_WORLD);
            int header_length = (num_messages+1)*sizeof(int);
            int msg_position = header_length;

            for (int i = 0; i < num_messages; i++) {
                std::unique_ptr<TimeWarpKernelMessage> msg = nullptr;

                // Unpack from aggregate buffer
                int msg_length = reinterpret_cast<int*>(message_buffer)[i + 1];
                auto msg_buffer = make_unique<char[]>(msg_length);
                MPI_Unpack(message_buffer, message_length, &msg_position, msg_buffer.get(), msg_length,
                           MPI_BYTE, MPI_COMM_WORLD);

                // Deserialize
                std::istringstream iss(std::string(msg_buffer.get(), msg_length));
                cereal::PortableBinaryInputArchive iarchive(iss);
                iarchive(msg);

                MessageType msg_type = msg->get_type();
                int msg_type_int = static_cast<int>(msg_type);
                msg_handler_by_msg_type_[msg_type_int](std::move(msg));
            }
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

bool TimeWarpMPICommunicationManager::barrierHoldStatus(){
    bool hold_status;

    barrier_hold_lock_.lock_shared();
    hold_status = barrier_hold_;
    barrier_hold_lock_.unlock_shared();

    return hold_status;
}

void TimeWarpMPICommunicationManager::barrierPause(){
    barrier_hold_lock_.lock();
    barrier_hold_ = true;
    barrier_hold_lock_.unlock();
}

// Don't need to lock here since the communication manager will be waiting at a barrier sync operation
void TimeWarpMPICommunicationManager::barrierResume(){
    barrier_hold_ = false;
}

} // namespace warped


