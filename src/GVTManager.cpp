#include "GVTManager.hpp"
#include "utility/memory.hpp"

namespace warped {

bool GVTManager::receiveGVTUpdate(std::unique_ptr<KernelMessage> kmsg) {
    auto msg = unique_cast<KernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;
    return false;
}

void GVTManager::sendGVTUpdate() {
    for (unsigned int i = 1; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gVT_);
        comm_manager_->sendMessage(std::move(update_msg));
    }
}

} // namespace warped
