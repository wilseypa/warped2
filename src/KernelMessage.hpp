#ifndef KERNEL_MESSAGE_HPP
#define KERNEL_MESSAGE_HPP

#include "serialization.hpp"

namespace warped {

struct KernelMessage {
    KernelMessage() = default;
    KernelMessage(unsigned int sender, unsigned int receiver) :
        senderID(sender),
        receiverID(receiver) {}

    unsigned int senderID;
    unsigned int receiverID;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(senderID, receiverID);
};

} // namespace warped

#endif
