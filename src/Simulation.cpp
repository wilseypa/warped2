#include "Simulation.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include "tclap/CmdLine.h"

#include "EventDispatcher.hpp"
#include "SequentialEventDispatcher.hpp"

namespace warped {

class Simulation::impl {
public:
    impl(const std::string& config_file_name, unsigned int max_sim_time)
        : config_file_name_(config_file_name), max_sim_time_(max_sim_time),
          dispatcher_(nullptr) {
        configureDispatcher();
    }

    impl(const std::string& model_description, int argc, const char* const* argv,
         const std::vector<TCLAP::Arg*>& cmd_line_args)
        : config_file_name_(""), max_sim_time_(0), dispatcher_(nullptr) {
        TCLAP::CmdLine cmd_line(model_description);
        try {

            TCLAP::ValueArg<std::string> config_arg("c", "config", "Warped configuration file",
                                                    false, config_file_name_, "file", cmd_line);
            TCLAP::ValueArg<unsigned int> sim_until_arg("t", "max-sim-time",
                                                        "specify a simulation end time",
                                                        false, max_sim_time_, "time", cmd_line);
            for (auto arg : cmd_line_args) {
                cmd_line.add(arg);
            }

            cmd_line.parse(argc, argv);
        } catch (TCLAP::ArgException& exc) {
            std::cerr << "error: " << exc.error() << " for arg " << exc.argId() << std::endl;
            throw;
        }
        configureDispatcher();
    }

    void configureDispatcher() {
        dispatcher_ = std::unique_ptr<EventDispatcher>{new SequentialEventDispatcher{max_sim_time_}};
    }

    std::string config_file_name_;
    unsigned int max_sim_time_;
    std::unique_ptr<EventDispatcher> dispatcher_;
};

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv,
                       const std::vector<TCLAP::Arg*>& cmd_line_args)
    : pimpl_(new impl(model_description, argc, argv, cmd_line_args)) {}

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv) {
    std::vector<TCLAP::Arg*> v;
    pimpl_ = std::unique_ptr<impl>(new impl(model_description, argc, argv, v));
}

Simulation::Simulation(const std::string& config_file_name, unsigned int max_sim_time)
    : pimpl_(new impl(config_file_name, max_sim_time)) {}

// We have to declare the default destructor here so that the compiler has
// access to the definition of impl
Simulation::~Simulation() = default;

void Simulation::simulate(std::vector<std::unique_ptr<SimulationObject>>& objects) {
    pimpl_->dispatcher_->startSimulation(objects);
}

} // namepsace warped