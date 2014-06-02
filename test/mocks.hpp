// This is a colelction of simple implementations of model-facing classes that
// are used to test various warped componenets.

#include "Event.hpp"
#include "ObjectState.hpp"
#include "serialization.hpp"
#include "SimulationObject.hpp"

WARPED_DEFINE_OBJECT_STATE_STRUCT(test_ObjectState) {
    test_ObjectState(int x=0): x(x) {}
    int x;
};

struct test_Event : public warped::Event {
    test_Event() = default;
    test_Event(const std::string& receiver_name, unsigned int receive_time, int x=0)
        : receiver_name_(receiver_name), receive_time_(receive_time), x(x) {}

    const std::string& receiverName() const {return receiver_name_;}
    unsigned int timestamp() const {return receive_time_;}

    int x;
    std::string receiver_name_;
    unsigned int receive_time_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(receiver_name_, receive_time_, x)
};
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(test_Event)

class test_SimulationObject : public warped::SimulationObject {
public:
    test_SimulationObject(const std::string& name, int x=0)
        : SimulationObject(name), state(x) {}

    warped::ObjectState& getState() { return state; }

    std::vector<std::unique_ptr<warped::Event>> receiveEvent(const warped::Event& event) {
        std::vector<std::unique_ptr<warped::Event>> v;
        v.emplace_back(new test_Event(event.receiverName(), event.timestamp()));
        return v;
    }

private:
    test_ObjectState state;
};