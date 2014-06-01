#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>

namespace warped {

// Events are passed between objects. They may contain data, and must be
// serializable. See serialization.hpp for info on serializing Events.
class Event {
public:
    virtual ~Event() {}

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;
};

} // namespace warped

#endif