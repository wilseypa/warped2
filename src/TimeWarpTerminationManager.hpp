#ifndef TERMINATION_HPP
#define TERMINATION_HPP

#include <memory>
#include <mutex>

#include "TimeWarpCommunicationManager.hpp"

namespace warped {

enum class State { ACTIVE, PASSIVE };

class TimeWarpTerminationManager {
public:

    TimeWarpTerminationManager(std::shared_ptr<TimeWarpCommunicationManager> comm_manager) :
        comm_manager_(comm_manager) {}

    void initialize(unsigned int num_worker_threads);

    // Send termination token
    bool sendTerminationToken(State state, unsigned int initiator);

    // Message handler for a termination token
    void receiveTerminationToken(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    // Notify all nodes to terminate, including self
    void sendTerminator();

    // Message handler for the terminator
    void receiveTerminator(std::unique_ptr<TimeWarpKernelMessage> kmsg);

    // Report thread passive
    void setThreadPassive(unsigned int thread_id);

    // Report thread active
    void setThreadActive(unsigned int thread_id);

    // Check to see if thread is passive
    bool threadPassive(unsigned int thread_id);

    // Check to see if all threads are passive
    bool nodePassive();

    // Check to see if we should terminate
    bool terminationStatus();

private:

    State state_ = State::ACTIVE;
    std::mutex state_lock_;
    State sticky_state_ = State::ACTIVE;

    std::shared_ptr<TimeWarpCommunicationManager> comm_manager_;

    std::unique_ptr<State []> state_by_thread_;
    unsigned int active_thread_count_;

    bool is_master_ = false;

    bool terminate_ = false;
};

struct TerminationToken : public TimeWarpKernelMessage {
    TerminationToken() = default;
    TerminationToken(unsigned int sender_id, unsigned int receiver_id, State state, unsigned int initiator) :
        TimeWarpKernelMessage(sender_id, receiver_id), state_(state), initiator_(initiator) {}

    State state_;

    unsigned int initiator_;

    MessageType get_type() { return MessageType::TerminationToken; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<TimeWarpKernelMessage>(this), state_,
                                         initiator_)
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


