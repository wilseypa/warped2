#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <memory>
#include <string>
#include <vector>

#include "Configuration.hpp"

namespace TCLAP { class Arg; }

namespace warped {
class SimulationObject;

class Simulation {
public:
    Simulation(const std::string& model_description, int argc, const char* const* argv);
    Simulation(const std::string& model_description, int argc, const char* const* argv,
               const std::vector<TCLAP::Arg*>& cmd_line_args);
    Simulation(const std::string& config_file_name, unsigned int max_time);

    void simulate(std::vector<SimulationObject*>& objects);

    // unsigned int getNumberOfPartitions(); // TODO
    // void simulate(std::vector<std::vector<std::unique_ptr<SimulationObject>>> objects); // TODO

private:
    Configuration config_;
};

} // namespace warped

#endif

