#include "SimulationObject.hpp"

#include <memory>
#include <string>
#include <vector>

#include "Event.hpp"
#include "Simulation.hpp"
#include "utility/memory.hpp"

namespace warped {

SimulationObject::SimulationObject(const std::string& name) : name_(name) {}

std::vector<std::unique_ptr<Event>> SimulationObject::createInitialEvents()  { return {}; }

FileStream& SimulationObject::getInputFileStream(const std::string& filename) {
    return Simulation::getFileStream(this, filename, std::ios_base::in);
}

FileStream& SimulationObject::getOutputFileStream(const std::string& filename) {
    return Simulation::getFileStream(this, filename, std::ios_base::out);
}

unsigned int SimulationObject::getSimulationTime() {
    return Simulation::getSimulationTime(this);
}

} // namespace warped
