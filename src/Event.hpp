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

    // Operator can be used in its current form only for handling anti-messages.
    // Receiver name comparison might needed for general use elsewhere.
    bool operator== (const Event &other) {
        return ((this->timestamp() == other.timestamp())
                && (this->sender_name_ == other.sender_name_)
                && (this->counter_ == other.counter_)
                && (this->send_time_ == other.send_time_));
    }

    bool operator!= (const Event &other) {
        return !(*this == other);
    }

    bool operator< (const Event &other) {
        bool is_less = false;
        if (this->timestamp() < other.timestamp()) {
            is_less = true;
        } else if (this->timestamp() == other.timestamp()) {
            if (this->send_time_ < other.send_time_) {
                is_less = true;
            } else if (this->send_time_ == other.send_time_) {
                if (this->sender_name_.compare(other.sender_name_) < 0) {
                    is_less = true;
                } else if (this->sender_name_.compare(other.sender_name_) == 0) {
                    if (this->counter_ < other.counter_) {
                        is_less = true;
                    } else {
                        is_less = false;
                    }
                } else {
                    is_less = false;
                }
            } else {
                is_less = false;
            }
        } else {
            is_less = false;
        }
        return is_less;
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

    // Event counter
    unsigned long counter_ = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_name_, event_type_, counter_, send_time_)

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
        counter_ = e->counter_;
    }

    const std::string& receiverName() const {return receiver_name_;}
    unsigned int timestamp() const {return receive_time_;}

    std::string receiver_name_;
    unsigned int receive_time_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(cereal::base_class<Event>(this), receiver_name_, receive_time_)
};

/* Compares two events to see if one has a receive time less than to the other
 *  21  */
struct compareEvents {
public:
    bool operator() (const std::shared_ptr<Event>& first,
                     const std::shared_ptr<Event>& second) const {

        bool is_less = false;
        if (first->timestamp() < second->timestamp()) {
            is_less = true;
        } else if (first->timestamp() == second->timestamp()) {
            if (first->send_time_ < second->send_time_) {
                is_less = true;
            } else if (first->send_time_ == second->send_time_) {
                if (first->sender_name_.compare(second->sender_name_) < 0) {
                    is_less = true;
                } else if (first->sender_name_.compare(second->sender_name_) == 0) {
                    if (first->counter_ < second->counter_) {
                        is_less = true;
                    } else if (first->counter_ == second->counter_) {
                        is_less = (first->event_type_ < second->event_type_) ? true : false;
                    } else {
                        is_less = false;
                    }
                } else {
                    is_less = false;
                }
            } else {
                is_less = false;
            }
        } else {
            is_less = false;
        }
        return is_less;
    }

};

} // namespace warped

#endif
