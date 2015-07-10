#include "TimeWarpCommunicationManager.hpp"

namespace warped {

void TimeWarpCommunicationManager::addRecvMessageHandler(MessageType msg_type,
    std::function<void(std::unique_ptr<TimeWarpKernelMessage>)> msg_handler) {

    int msg_type_int = static_cast<int>(msg_type);
    msg_handler_by_msg_type_.insert(std::make_pair(msg_type_int, msg_handler));
}

void TimeWarpCommunicationManager::deliverReceivedMessages() {
    auto msg = getMessage();
    while (msg.get() != nullptr) {
        MessageType msg_type = msg->get_type();
        int msg_type_int = static_cast<int>(msg_type);

        auto msg_handler = msg_handler_by_msg_type_[msg_type_int];
        msg_handler(std::move(msg));

        msg = getMessage();
    }
}

void TimeWarpCommunicationManager::initializeObjectMap(const std::vector<std::vector<SimulationObject*>>& objects) {
    unsigned int partition_id = 0;
    for (auto& partition : objects) {
        for (auto& ob : partition) {
            node_id_by_object_name_[ob->name_] = partition_id;
        }
        partition_id++;
    }
}

unsigned int TimeWarpCommunicationManager::getNodeID(std::string object_name) {
    return node_id_by_object_name_[object_name];
}

} // namespace warped

