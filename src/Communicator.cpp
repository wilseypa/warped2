#include "Communicator.hpp"
#include "MatternGVTManager.hpp"
#include "TimeWarpEventDispatcher.hpp"

namespace warped {

unsigned int Communicator::initCommunicationManager(std::unique_ptr<CommunicationManager> cm) {
    comm_manager_ = std::move(cm);
    comm_manager_->initialize();
    return comm_manager_->getNumProcesses();
}

void Communicator::finalizeCommunicationManager() {
    comm_manager_->finalize();
}

void Communicator::dispatchReceivedMessages() {
    auto msg = comm_manager_->recvMessage();
    while (msg.get() != nullptr) {
        switch(msg->get_type()) {
            case MessageType::MatternGVTToken:
                static_cast<MatternGVTManager&>(*this).receiveMessage(std::move(msg));
                break;
            case MessageType::EventMessage:
                static_cast<TimeWarpEventDispatcher&>(*this).receiveMessage(std::move(msg));
                break;
            default:
                break;
        }
        msg = comm_manager_->recvMessage();
    }
}


} // namespace warped
