#ifndef WARPED_EVENT_H
#define WARPED_EVENT_H

#include <string>

namespace warped {

class Event {
public:
    Event(const std::string& receiverName)
        : receiverName(receiverName) {}
    virtual ~Event() {}

    const std::string receiverName;

};

} // namespace warped

#endif