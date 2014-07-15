#ifndef STRAGGLER_EVENT_HPP
#define STRAGGLER_EVENT_HPP

namespace warped {

// This is a Straggler event class
class StragglerEvent : public Event {
public:
    StragglerEvent() = default;
    SignalEvent(const std::string& receiver_name, unsigned int timestamp)
        : receiver_name_(receiver_name), timestamp_(timestamp)
    {}

    const std::string& receiverName() const { return receiver_name_; }
    unsigned int timestamp() const { return timestamp_; }

    std::string receiver_name_;
    unsigned int timestamp_;
    const std::vector<const std::unique_ptr<Event>> events_to_cancel_;
};

} // namespace warped

#endif
