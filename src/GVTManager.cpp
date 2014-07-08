#include "GVTManager.hpp"
#include "utility/memory.hpp"

namespace warped {

void GVTManager::receiveGVTUpdate(std::unique_ptr<GVTUpdateMessage> msg) {
    gVT_ = msg->new_gvt;
}

void GVTManager::sendGVTUpdate() {
    for (unsigned int i = 1; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gVT_);
        comm_manager_->sendMessage(std::move(update_msg));
    }
}

} // namespace warped
