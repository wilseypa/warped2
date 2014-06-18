#ifndef KERNEL_MESSAGE_HPP
#define KERNEL_MESSAGE_HPP

#include "serialization.hpp"

namespace warped {

enum class MessageType {
    EventMessage,
    MatternGVTToken
};

struct KernelMessage {
    KernelMessage() = default;
    KernelMessage(unsigned int sender, unsigned int receiver, MessageType mt) :
        sender_id(sender),
        receiver_id(receiver),
        message_type(mt) {}

    unsigned int sender_id;
    unsigned int receiver_id;

    MessageType message_type;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_id, receiver_id, message_type)

};

} // namespace warped

#endif
