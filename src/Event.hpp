#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>

namespace warped {

enum class EventType : bool {
    POSITIVE,
    NEGATIVE,
};

// Events are passed between objects. They may contain data, and must be
// serializable. See serialization.hpp for info on serializing Events.
class Event {
public:
    virtual ~Event() {}

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The name of the SimualtionObject that sends this event.
    virtual const std::string& senderName() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;

    EventType event_type() { return event_type_; }

    void setNegative() { event_type_ = EventType::NEGATIVE; }

private:
    EventType event_type_ = EventType::POSITIVE;
};

} // namespace warped

#endif
