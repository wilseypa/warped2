#ifndef TERMINATION_HPP
#define TERMINATION_HPP

#include <memory>
#include <atomic>

#include "TimeWarpCommunicationManager.hpp"

namespace warped {

enum class State { ACTIVE, PASSIVE };

class TimeWarpTerminationManager {
public:

    TimeWarpTerminationManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager) :
        comm_manager_(comm_manager) {}

    void initialize(unsigned int num_worker_threads);

    void sendTerminationToken(State state, unsigned int initiator);

    void receiveTerminationToken(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void sendTerminator();

    void receiveTerminator(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    void setThreadPassive(unsigned int thread_id);

    void setThreadActive(unsigned int thread_id);

    bool threadPassive(unsigned int thread_id);

    bool nodePassive();

    bool terminationStatus();

private:

    State state_ = State::ACTIVE;
    State sticky_state_ = State::ACTIVE;

    std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    std::unique_ptr<State []> state_by_thread_;
    std::atomic<unsigned int> active_thread_count_;

    bool terminate_ = false;
};

struct TerminationToken : public TimeWarpKernelMessage {
    TerminationToken() = default;
    TerminationToken(unsigned int sender_id, unsigned int receiver_id, State state,
        unsigned int initiator) :
        TimeWarpKernelMessage(sender_id, receiver_id), state_(state), initiator_(initiator) {}

    State state_;

    unsigned int initiator_;

    MessageType get_type() { return MessageType::TerminationToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), state_, initiator_)
};

struct Terminator : public TimeWarpKernelMessage {
    Terminator() = default;
    Terminator(unsigned int sender_id, unsigned int receiver_id) :
        TimeWarpKernelMessage(sender_id, receiver_id) {}

    MessageType get_type() { return MessageType::Terminator; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this))
};

} // namespace warped

#endif


