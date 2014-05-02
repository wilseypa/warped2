#include "Simulation.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include "tclap/CmdLine.h"

namespace warped {

class Simulation::impl {
public:
    impl(const std::string& config_file_name, unsigned int simulate_until)
        : config_file_name_(config_file_name), simulate_until_(simulate_until) {}

    impl(const std::string& model_description, int argc, const char* const* argv,
         const std::vector<TCLAP::Arg*>& cmd_line_args)
        : config_file_name_(), simulate_until_() {
        TCLAP::CmdLine cmd_line(model_description);
        try {

            TCLAP::ValueArg<std::string> config_arg("c", "config", "Warped2 configuration file",
                                                    false, config_file_name_, "file", cmd_line);
            TCLAP::ValueArg<unsigned int> sim_until_arg("u", "simulate-until",
                                                        "specify a simulation end time",
                                                        false, simulate_until_, "time", cmd_line);
            for (auto arg : cmd_line_args) {
                cmd_line.add(arg);
            }

            cmd_line.parse(argc, argv);
        } catch (TCLAP::ArgException& exc) {
            std::cerr << "error: " << exc.error() << " for arg " << exc.argId() << std::endl;
            throw;
        }
    }

    std::string config_file_name_;
    unsigned int simulate_until_;
};

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv,
                       const std::vector<TCLAP::Arg*>& cmd_line_args)
    : pimpl_(new impl(model_description, argc, argv, cmd_line_args)) {}

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv) {
    std::vector<TCLAP::Arg*> v;
    pimpl_ = std::unique_ptr<impl>(new impl(model_description, argc, argv, v));
}

Simulation::Simulation(const std::string& config_file_name, unsigned int simulate_until)
    : pimpl_(new impl(config_file_name, simulate_until)) {}

// We have to declare the default destructor here so that the compiler has
// access to the definition of impl
Simulation::~Simulation() = default;

} // namepsace warped