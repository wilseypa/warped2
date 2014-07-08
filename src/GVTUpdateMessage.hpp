#ifndef GVT_UPDATE_MESSAGE_HPP
#define GVT_UPDATE_MESSAGE_HPP

#include "KernelMessage.hpp"

namespace warped {

struct GVTUpdateMessage : public KernelMessage {
    GVTUpdateMessage() = default;
    GVTUpdateMessage(unsigned int sender_id, unsigned int receiver_id, unsigned int gvt) :
        KernelMessage(sender_id, receiver_id), new_gvt(gvt) {}

    unsigned int new_gvt;

    MessageType get_type() { return MessageType::GVTUpdateMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<KernelMessage>(this), new_gvt)
};

}

#endif
