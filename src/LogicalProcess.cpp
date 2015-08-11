#include "LogicalProcess.hpp"

#include <memory>
#include <string>
#include <vector>

#include "Event.hpp"
#include "Simulation.hpp"
#include "utility/memory.hpp"

namespace warped {

LogicalProcess::LogicalProcess(const std::string& name) : name_(name) {}

std::vector<std::shared_ptr<Event>> LogicalProcess::initializeLP()  { return {}; }

FileStream& LogicalProcess::getInputFileStream(const std::string& filename) {
    return Simulation::getFileStream(this, filename, std::ios_base::in, nullptr);
}

FileStream& LogicalProcess::getOutputFileStream(const std::string& filename,
    std::shared_ptr<Event> this_event) {

    return Simulation::getFileStream(this, filename, std::ios_base::out, this_event);
}

} // namespace warped
