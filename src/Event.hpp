#ifndef WARPED_EVENT_HPP
#define WARPED_EVENT_HPP

#include <string>
#include "serialization.hpp"

#define ROLLBACK_CNT_ACTIVE   0  // activate if rollback count becomes relevant

namespace warped {

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
#if ROLLBACK_CNT_ACTIVE
        return ((this->timestamp() == other.timestamp())
                && (this->sender_name_ == other.sender_name_)
                && (this->event_type_ != other.event_type_)
                && (this->rollback_cnt_ == other.rollback_cnt_));
#else
        return ((this->timestamp() == other.timestamp())
                && (this->sender_name_ == other.sender_name_)
                && (this->event_type_ != other.event_type_));
#endif
    }

    bool operator!= (const Event &other) {
        return !(*this == other);
    }

    bool operator< (const Event &other) {
        if (this->timestamp() < other.timestamp()) {
            return true;
        } else if (this->timestamp() == other.timestamp()) {
            if (this->sender_name_.compare(other.sender_name_) < 0) {
                return true;
            } else if (this->sender_name_.compare(other.sender_name_) == 0) {
#if ROLLBACK_CNT_ACTIVE
                if (this->rollback_cnt_ < other.rollback_cnt_) {
                    return true;
                } else {
                    return false;
                }
#else
                return false;
#endif
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    // The name of the SimualtionObject that should receive this event.
    virtual const std::string& receiverName() const = 0;

    // The timestamp of when the event should be received.
    virtual unsigned int timestamp() const = 0;

    // The name of the SimualtionObject that sends this event.
    std::string sender_name_;

    // Event type - positive or negative
    EventType event_type_ = EventType::POSITIVE;

    // Rollback id
    unsigned int rollback_cnt_ = 0;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(sender_name_, event_type_, rollback_cnt_)

};

} // namespace warped

#endif
