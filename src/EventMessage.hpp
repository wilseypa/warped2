#ifndef EVENT_MESSAGE_HPP
#define EVENT_MESSAGE_HPP

#include <memory> // for std::unique_ptr
#include <string> // for std::string

#include "MatternGVTManager.hpp" // for MatternMsgColor

namespace warped {

struct EventMessage : public KernelMessage {
    EventMessage() = default;
    EventMessage(unsigned int sender, unsigned int receiver, std::unique_ptr<Event> e,
        std::string add = "") :
        KernelMessage(sender, receiver),
        additional_info(add),
        event(std::move(e)) {}

    std::string additional_info;
    std::unique_ptr<Event> event;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(additional_info, event)
};

} // namespace warped

#endif
