#ifndef WARPED_EVENT_H
#define WARPED_EVENT_H

#include <string>

namespace warped {

class Event {
public:
    virtual ~Event() {}

    virtual const std::string& get_receiver_name() = 0;
    virtual unsigned int get_receive_time() = 0;
};

} // namespace warped

#endif