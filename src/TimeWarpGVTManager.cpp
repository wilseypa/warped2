#include "TimeWarpGVTManager.hpp"
#include "utility/memory.hpp"

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(warped::GVTUpdateMessage)

namespace warped {

MessageFlags TimeWarpGVTManager::receiveGVTUpdate(std::unique_ptr<TimeWarpKernelMessage> kmsg) {
    auto msg = unique_cast<TimeWarpKernelMessage, GVTUpdateMessage>(std::move(kmsg));
    gVT_ = msg->new_gvt;
    return MessageFlags::GVTUpdate;
}

void TimeWarpGVTManager::sendGVTUpdate() {
    for (unsigned int i = 1; i < comm_manager_->getNumProcesses(); i++) {
        auto update_msg = make_unique<GVTUpdateMessage>(0, i, gVT_);
        comm_manager_->sendMessage(std::move(update_msg));
    }
}

} // namespace warped
