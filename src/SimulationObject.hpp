#ifndef SIMULATION_OBJECT_HPP
#define SIMULATION_OBJECT_HPP

#include <string>
#include <vector>
#include <memory>

namespace warped {

class Event;
struct ObjectState;

// SimulationObjects are the core of the simulation. All models must define at
// least one class implementing this interface. Each SimulationObject has a
// name_ that is used to identify the object. It must be unique across all
// SimulationObjects.
class SimulationObject {
public:
    SimulationObject(const std::string& name);
    virtual ~SimulationObject() {}

    // Return the state of this object.
    //
    // Any state that is mutable once the simulation has begun must be stored
    // in an ObjectState class. This object is saved and restored repeatedly
    // during the course of the simulation.
    virtual ObjectState& getState() = 0;

    // This is the main function of the SimulationObject which processes incoming events.
    //
    // It is called when an object is sent an event. The receive time of the
    // event is the current simulation time. The object may create new events
    // and return then as a vector, or return an empty vector if no events are
    // created. Events must not be created with a timestamp less than the
    // current simulation time.
    virtual std::vector<std::unique_ptr<Event>> receiveEvent(const Event& event) = 0;

    // Create events before the simulation starts.
    //
    // This is an optional method that is called before the simulation begins.
    virtual std::vector<std::unique_ptr<Event>> createInitialEvents();

    const std::string name_;
};

} // namespace warped

#endif
