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

FileStream& SimulationObject::getFileStream(const std::string& filename,
    std::ios_base::openmode mode) {

    return Simulation::getFileStream(this, filename, mode);
}

} // namespace warped
