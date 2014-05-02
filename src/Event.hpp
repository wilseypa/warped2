#ifndef WARPED_EVENT_H
#define WARPED_EVENT_H

#include <string>

namespace warped {

class Event {
public:
    Event(const std::string& receiver_name)
        : receiver_name_(receiver_name) {}
    virtual ~Event() {}

    const std::string receiver_name_;

};

} // namespace warped

#endif