#ifndef SIMULATION_OBJECT_H
#define SIMULATION_OBJECT_H

#include <string>

class ObjectState;

class SimulationObject {
public:
    SimulationObject(const std::string& name) : name(name) {}
    virtual ~SimulationObject() {}

    virtual ObjectState& getState() = 0;

    const std::string name;
};

#endif
