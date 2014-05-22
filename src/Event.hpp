#ifndef WARPED_EVENT_H
#define WARPED_EVENT_H

#include <string>

namespace warped {

// Events are passed between objects. They may contain data, and must be
// serializable. See serialization.hpp for info on serializing Events.
class Event {
public:
    virtual ~Event() {}

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& get_receiver_name() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int get_receive_time() const = 0;
};

} // namespace warped

#endif