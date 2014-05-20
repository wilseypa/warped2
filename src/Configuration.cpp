#include "Configuration.hpp"

#include <memory>
#include <string>
#include <vector>

#include "tclap/CmdLine.h"

#include "SequentialEventDispatcher.hpp"

namespace warped {

Configuration::Configuration(const std::string& config_file_name, unsigned int max_sim_time)
    : config_file_name_(config_file_name), max_sim_time_(max_sim_time) {}

Configuration::Configuration(const std::string& model_description, int argc,
                             const char* const* argv,
                             const std::vector<TCLAP::Arg*>& cmd_line_args)
    : config_file_name_(""), max_sim_time_(0) { init(model_description, argc, argv, cmd_line_args); }

Configuration::Configuration(const std::string& model_description, int argc,
                             const char* const* argv)
    : config_file_name_(""), max_sim_time_(0) {
    std::vector<TCLAP::Arg*> v;
    init(model_description, argc, argv, v);
}

void Configuration::init(const std::string& model_description, int argc, const char* const* argv,
                         const std::vector<TCLAP::Arg*>& cmd_line_args) {
    TCLAP::CmdLine cmd_line(model_description);

    TCLAP::ValueArg<std::string> config_arg("c", "config", "Warped configuration file",
                                            false, config_file_name_, "file", cmd_line);
    TCLAP::ValueArg<unsigned int> max_sim_time_arg("t", "max-sim-time",
                                                   "specify a simulation end time",
                                                   false, max_sim_time_, "time", cmd_line);
    for (auto arg : cmd_line_args) {
        cmd_line.add(arg);
    }

    cmd_line.parse(argc, argv);

    max_sim_time_ = max_sim_time_arg.getValue();
    config_file_name_ = config_arg.getValue();

}

std::unique_ptr<EventDispatcher> Configuration::make_dispatcher() {
    return std::unique_ptr<EventDispatcher> {new SequentialEventDispatcher{max_sim_time_}};
}

} // namespace warped