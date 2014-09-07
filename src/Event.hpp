#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>

#include "serialization.hpp"

namespace warped {

enum class EventType : bool {
    POSITIVE,
    NEGATIVE,
};

// Events are passed between objects. They may contain data, and must be
// serializable. See serialization.hpp for info on serializing Events.
class Event {
public:
    Event() = default;
    virtual ~Event() {}

    bool operator== (const Event &other) {
        return (this->receiverName() == other.receiverName()
                && this->sender_name_ == other.sender_name_
                && this->timestamp() == other.timestamp()
                && this->rollback_cnt_ == other.rollback_cnt_);
    }

    bool operator!= (const Event &other) {
        return !(*this == other);
    }

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The name of the SimualtionObject that sends this event.
    std::string sender_name_ = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;

    // Event type - positive or negative
    EventType event_type_ = EventType::POSITIVE;

    // Rollback id
    unsigned int rollback_cnt_ = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(event_type_)

};

} // namespace warped

#endif
