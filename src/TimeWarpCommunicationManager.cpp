#include "TimeWarpCommunicationManager.hpp"

namespace warped {

void TimeWarpCommunicationManager::addRecvMessageHandler(MessageType msg_type,
    std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler) {

    int msg_type_int = static_cast<int>(msg_type);
    msg_handler_by_msg_type_.insert(std::make_pair(msg_type_int, msg_handler));
}

void TimeWarpCommunicationManager::initializeLPMap(const std::vector<std::vector<LogicalProcess*>>& lps) {
    unsigned int partition_id = 0;
    for (auto& partition : lps) {
        for (auto& lp : partition) {
            node_id_by_lp_name_[lp->name_] = partition_id;
        }
        partition_id++;
    }
}

unsigned int TimeWarpCommunicationManager::getNodeID(std::string lp_name) {
    return node_id_by_lp_name_[lp_name];
}

} // namespace warped

