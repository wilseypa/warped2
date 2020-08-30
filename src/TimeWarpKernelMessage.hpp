#ifndef KERNEL_MESSAGE_HPP
#define KERNEL_MESSAGE_HPP

#include "serialization.hpp"

/* TimeWarpKernelMessage provides a base class for any message type. Any sub class must implement
 * a get_type() method so that the type can be determined.
 */

namespace warped {

enum class MessageType {
    EventMessage,
    MatternGVTToken,
    GVTUpdateMessage,
    GVTSynchTrigger,
    TerminationToken,
    Terminator
};

struct TimeWarpKernelMessage {
    TimeWarpKernelMessage() = default;
    virtual ~TimeWarpKernelMessage() = default;

    TimeWarpKernelMessage(unsigned int sender, unsigned int receiver) :
        sender_id(sender), receiver_id(receiver) {}

    unsigned int sender_id;
    unsigned int receiver_id;

    virtual MessageType get_type() = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_id, receiver_id)

};

} // namespace warped

#endif
