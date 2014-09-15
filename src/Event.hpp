#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>

#include "serialization.hpp"

namespace warped {

enum class EventType : bool {
    NEGATIVE = 0,
    POSITIVE
};

// Events are passed between objects. They may contain data, and must be
// serializable. See serialization.hpp for info on serializing Events.
class Event {
public:
    Event() = default;
    virtual ~Event() {}

    // Operator can be used in its current form only for handling anti-messages.
    // Receiver name comparison might needed for general use elsewhere.
    bool operator== (const Event &other) {
        return (this->sender_name_ == other.sender_name_
                && this->timestamp() == other.timestamp()
                && this->rollback_cnt_ == other.rollback_cnt_);
    }

    bool operator!= (const Event &other) {
        return !(*this == other);
    }

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;

    // The name of the SimualtionObject that sends this event.
    std::string sender_name_;

    // Event type - positive or negative
    EventType event_type_ = EventType::POSITIVE;

    // Rollback id
    unsigned int rollback_cnt_ = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_name_, event_type_, rollback_cnt_)

};

} // namespace warped

#endif
