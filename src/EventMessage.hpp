#ifndef EVENT_MESSAGE_HPP
#define EVENT_MESSAGE_HPP

#include <memory> // for std::unique_ptr
#include <string> // for std::string

#include "KernelMessage.hpp"
#include "Event.hpp"

namespace warped {

enum class MatternColor;

struct EventMessage : public KernelMessage {
    EventMessage() = default;
    EventMessage(unsigned int sender, unsigned int receiver, std::unique_ptr<Event> e,
        int c) :
        KernelMessage(sender, receiver),
        event(std::move(e)),
        gvt_mattern_color(c) {}

    std::unique_ptr<Event> event;
    int gvt_mattern_color; // 0 for white, 1 for red

    MessageType get_type() { return MessageType::EventMessage; }

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<KernelMessage>(this), event,
        gvt_mattern_color)
};

} // namespace warped

#endif
