#ifndef STRAGGLER_EVENT_HPP
#define STRAGGLER_EVENT_HPP

namespace warped {

// This is a Straggler event class
class StragglerEvent : public Event {
public:
    StragglerEvent() = default;
    StragglerEvent( const std::string& receiver_name, 
                    const std::string& sender_name, 
                    unsigned int timestamp )
        :   receiver_name_(receiver_name), 
            sender_name_(sender_name), 
            timestamp_(timestamp)
    {}

    const std::string& receiverName() const { return receiver_name_; }
    const std::string& senderName() const { return sender_name_; }
    unsigned int timestamp() const { return timestamp_; }

    std::string receiver_name_;
    std::string sender_name_;
    unsigned int timestamp_;
    const std::vector<const std::unique_ptr<Event>> events_to_cancel_;
};

} // namespace warped

#endif
