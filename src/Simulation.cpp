#include "Simulation.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include "EventDispatcher.hpp"

class SimulationObject;

namespace warped {

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv,
                       const std::vector<TCLAP::Arg*>& cmd_line_args)
    : config_(model_description, argc, argv, cmd_line_args) {}

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv)
    : config_(model_description, argc, argv) {}

Simulation::Simulation(const std::string& config_file_name, unsigned int max_sim_time)
    : config_(config_file_name, max_sim_time) {}

void Simulation::simulate(std::vector<SimulationObject*>& objects) {
    std::unique_ptr<EventDispatcher> dispatcher = config_.makeDispatcher();
    dispatcher->startSimulation(objects);
}

} // namepsace warped