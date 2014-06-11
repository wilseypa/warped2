#ifndef TERMINATION_MANAGER_HPP
#define TERMINATION_MANAGER_HPP

#include "serialization.hpp"

#include <memory>

namespace warped {

enum class TerminationState { ACTIVE, PASSIVE };

struct TerminationControlMessage {
    TerminationControlMessage(TerminationState s) :
        state(s) {}

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(state)

    TerminationState state;
};

class TerminationManager {
public:
    TerminationManager(uint32_t node_id, uint32_t num_partitions) :
        node_id_(node_id),
        num_partitions_(num_partitions) {}

    void sendInitialControlMessage();

    void sendControlMessage(std::unique_ptr<TerminationControlMessage> msg, uint32_t node_id);

    void receiveControlMessage(std::unique_ptr<TerminationControlMessage> msg);

    void terminate();

private:

    // State of this process, this is either ACTIVE or PASSIVE
    TerminationState state_ = TerminationState::PASSIVE;

    TerminationState sticky_state_indicator_ = TerminationState::PASSIVE;

    uint32_t node_id_;

    uint32_t num_partitions_;
};

} // warped namespace

#endif
