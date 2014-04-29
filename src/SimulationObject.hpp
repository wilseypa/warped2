#ifndef SIMULATION_OBJECT_H
#define SIMULATION_OBJECT_H

#include <string>

class ObjectState;

class SimObject {
public:
    SimObject(const std::string& name) : name(name) {}
    virtual ~SimObject();

    virtual ObjectState& getState() = 0;

    const std::string name;
};

#endif
