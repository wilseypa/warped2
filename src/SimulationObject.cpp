#include "SimulationObject.hpp"

#include <memory>
#include <string>
#include <vector>

#include "Event.hpp"

namespace warped {

SimulationObject::SimulationObject(const std::string& name) : name_(name) {}

std::vector<std::unique_ptr<Event>> SimulationObject::createInitialEvents()  { return {}; }

} // namespace warped
