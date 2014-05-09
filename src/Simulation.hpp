#ifndef SIMULATION_H
#define SIMULATION_H

#include <memory>
#include <string>
#include <vector>

#include "SimulationObject.hpp"

namespace TCLAP { class Arg; }

namespace warped {

class Simulation {
public:
    Simulation(const std::string& model_description, int argc, const char* const* argv);
    Simulation(const std::string& model_description, int argc, const char* const* argv,
               const std::vector<TCLAP::Arg*>& cmd_line_args);
    Simulation(const std::string& config_file_name, unsigned int max_time);
    ~Simulation();

    void simulate(std::vector<std::unique_ptr<SimulationObject>>& objects);

    // unsigned int getNumberOfPartitions(); // TODO
    // void simulate(std::vector<std::vector<std::unique_ptr<SimulationObject>>> objects); // TODO

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace warped

#endif

