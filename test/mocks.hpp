// This is a colelction of simple implementations of model-facing classes that
// are used to test various warped componenets.

#include "Event.hpp"
#include "LPState.hpp"
#include "serialization.hpp"
#include "LogicalProcess.hpp"

enum enum_type {
    VALUE1,
    VALUE2,
};

struct some_type {
    double z;
    enum_type a;
};

WARPED_DEFINE_LP_STATE_STRUCT(test_LPState) {
    test_LPState(int x=0): x(x) {
        y = std::make_shared<int>(0);
        *y = 0;
        a_map = std::make_shared<std::map<unsigned int, std::shared_ptr<enum_type>>>();
    }

    test_LPState(const test_LPState& other) {
        x = other.x;
        y = std::make_shared<int>(*other.y);

        a_map = std::make_shared<std::map<unsigned int, std::shared_ptr<enum_type>>>();
        for (auto it = other.a_map->begin(); it != other.a_map->end(); it++) {
            auto new_val = std::make_shared<enum_type>(*it->second);
            a_map->insert(a_map->begin(), std::pair<unsigned int, std::shared_ptr<enum_type>>(it->first , new_val));
        }
    }

    int x;
    std::shared_ptr<int> y;
    std::shared_ptr<std::map<unsigned int, std::shared_ptr<enum_type>>> a_map;
};

struct test_Event : public warped::Event {
    test_Event() = default;
    test_Event(const std::string& receiver_name, unsigned int receive_time)
        : receiver_name_(receiver_name), receive_time_(receive_time) {}

    test_Event(const std::string& receiver_name, unsigned int receive_time, bool is_positive)
                : receiver_name_(receiver_name), receive_time_(receive_time) {
        event_type_ = is_positive ? warped::EventType::POSITIVE : warped::EventType::NEGATIVE;
    }

    const std::string& receiverName() const {return receiver_name_;}

    unsigned int timestamp() const {return receive_time_;}

    std::string receiver_name_;
    unsigned int receive_time_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(receiver_name_, sender_name_, receive_time_)
};
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(test_Event)

class test_LogicalProcess : public warped::LogicalProcess {
public:
    test_LogicalProcess(const std::string& name, int x=0)
        : LogicalProcess(name), state(x) {}

    warped::LPState& getState() { return state; }

    std::vector<std::shared_ptr<warped::Event>> receiveEvent(const warped::Event& event) {
        std::vector<std::shared_ptr<warped::Event>> v;
        v.emplace_back(new test_Event(event.receiverName(), event.timestamp()));
        return v;
    }

private:
    test_LPState state;
};


