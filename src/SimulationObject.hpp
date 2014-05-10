#ifndef SIMULATION_OBJECT_H
#define SIMULATION_OBJECT_H

#include <string>
#include <vector>
#include <memory>

namespace warped {
    
class Event;
class ObjectState;

class SimulationObject {
public:
    SimulationObject(const std::string& name) : name_(name) {}
    virtual ~SimulationObject() {}

    virtual ObjectState& getState() = 0;

    virtual std::vector<std::unique_ptr<Event>> receiveEvent(const Event& event) = 0;
    virtual std::vector<std::unique_ptr<Event>> createInitialEvents()  { return {}; }

    const std::string name_;
};

} // namespace warped

#endif
