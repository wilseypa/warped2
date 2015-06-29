#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>
#include "serialization.hpp"

namespace warped {

struct compareEvents;

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

    bool operator== (const Event &other) {
        return ((this->timestamp() == other.timestamp())
                && (this->send_time_ == other.send_time_)
                && (this->sender_name_ == other.sender_name_)
                && (this->generation_ == other.generation_));
    }

    bool operator< (const Event &other) {
        return  (this->timestamp() < other.timestamp()) ? true :
                ((this->timestamp() != other.timestamp()) ? false :
                  ((this->send_time_ < other.send_time_) ? true :
                  ((this->send_time_ != other.send_time_) ? false :
                    ((this->sender_name_ < other.sender_name_) ? true :
                    ((this->sender_name_ != other.sender_name_) ? false :
                      ((this->generation_ < other.generation_) ? true :
                      ((this->generation_ != other.generation_) ? false : false)))))));
    }

    bool operator<= (const Event &other) {
        return (*this < other) || (*this == other);
    }

    bool operator>= (const Event &other) {
        return !(*this < other);
    }

    bool operator> (const Event &other) {
        return !(*this <= other);
    }

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;

    // The name of the SimualtionObject that sends this event.
    std::string sender_name_;

    // Event type - positive or negative
    EventType event_type_ = EventType::POSITIVE;

    // Send time
    unsigned int send_time_ = 0;

    // For differentiating same events which is caused by
    //  anti-message + regeneration of event.
    unsigned long long generation_ = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_name_, event_type_, send_time_, generation_)

};

class NegativeEvent : public Event {
public:
    NegativeEvent() = default;
    NegativeEvent(std::shared_ptr<Event> e) {
        receiver_name_ = e->receiverName();
        receive_time_ = e->timestamp();
        sender_name_ = e->sender_name_;
        send_time_ = e->send_time_;
        event_type_ = EventType::NEGATIVE;
        generation_ = e->generation_;
    }

    const std::string& receiverName() const {return receiver_name_;}
    unsigned int timestamp() const {return receive_time_;}

    std::string receiver_name_;
    unsigned int receive_time_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<Event>(this), receiver_name_, receive_time_)
};

// Initial event used with the initial state save of all objects
class InitialEvent : public Event {
public:
    InitialEvent() {
        sender_name_ = "";
        send_time_ = 0;
        generation_ = 0;
   }

    const std::string& receiverName() const { return receiver_name_; }
    unsigned int timestamp() const { return 0; }

    std::string receiver_name_ = "";
};

/* Compares two events to see if one has a receive time less than to the other */
struct compareEvents {
public:
    bool operator() (const std::shared_ptr<Event>& first,
                     const std::shared_ptr<Event>& second) const {
        return  (first->timestamp() < second->timestamp()) ? true :
                ((first->timestamp() != second->timestamp()) ? false :
                  ((first->send_time_ < second->send_time_) ? true :
                  ((first->send_time_ != second->send_time_) ? false :
                    ((first->sender_name_ < second->sender_name_) ? true :
                    ((first->sender_name_ != second->sender_name_) ? false :
                      ((first->generation_ < second->generation_) ? true :
                      ((first->generation_ != second->generation_) ? false :
                        ((first->event_type_ < second->event_type_) ? true :
                        ((first->event_type_ != second->event_type_) ? false : false)))))))));
    }

};

} // namespace warped

#endif
