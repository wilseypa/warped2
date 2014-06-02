#include "Simulation.hpp"

#include <memory>
#include <vector>

#include "Configuration.hpp"
#include "EventDispatcher.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"

namespace warped {

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv,
                       const std::vector<TCLAP::Arg*>& cmd_line_args)
    : config_(model_description, argc, argv, cmd_line_args) {}

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv)
    : config_(model_description, argc, argv) {}

Simulation::Simulation(const std::string& config_file_name, unsigned int max_sim_time)
    : config_(config_file_name, max_sim_time) {}

void Simulation::simulate(const std::vector<SimulationObject*>& objects) {
    std::unique_ptr<EventDispatcher> dispatcher = config_.makeDispatcher();
    // TODO: get the actual number of partitions for parallel simulations
    unsigned int num_partitions = 1;
    auto partitioned_objects = config_.makePartitioner()->partition(objects, num_partitions);
    dispatcher->startSimulation(partitioned_objects);
}

} // namepsace warped