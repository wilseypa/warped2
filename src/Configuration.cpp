#include "Configuration.hpp"

#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "tclap/CmdLine.h"
#include "json/json.h"

#include "SequentialEventDispatcher.hpp"

namespace {
const static std::string DEFAULT_CONFIG = R"x({
                // Valid options are "sequential" and "time-warp"
                "simulation-type": "sequential"

                })x";


// Recursively copy the values of b into a. Both a and b must be objects.
void update(Json::Value& a, Json::Value& b) {
    if (!a.isObject() || !b.isObject()) { return; }

    for (const auto& key : b.getMemberNames()) {
        if (a[key].isObject()) {
            update(a[key], b[key]);
        } else {
            a[key] = b[key];
        }
    }
}

void addParameters(TCLAP::CmdLine& cmd_line, const std::vector<TCLAP::Arg*>& args) {
    for (auto arg : args) {
        cmd_line.add(arg);
    }
}

Json::Value parseJsonFile(std::string filename) {
    Json::Reader reader;
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error(std::string("Could not open configuration file ") + filename);
    }
    Json::Value root;
    auto success = reader.parse(input, root);
    input.close();
    if (!success) {
        throw std::runtime_error(std::string("Failed to parse configuration\n") +
                                 reader.getFormattedErrorMessages());
    }
    return root;
}

} // namespace

namespace warped {

Configuration::Configuration(const std::string& config_file_name, unsigned int max_sim_time)
    : config_file_name_(config_file_name), max_sim_time_(max_sim_time), root_(nullptr) {}

Configuration::Configuration(const std::string& model_description, int argc,
                             const char* const* argv,
                             const std::vector<TCLAP::Arg*>& cmd_line_args)
    : config_file_name_(""), max_sim_time_(0), root_(new Json::Value)
{ init(model_description, argc, argv, cmd_line_args); }

Configuration::Configuration(const std::string& model_description, int argc,
                             const char* const* argv)
    : config_file_name_(""), max_sim_time_(0), root_(new Json::Value)
{ init(model_description, argc, argv, {}); }

Configuration::~Configuration() = default;

void Configuration::init(const std::string& model_description, int argc, const char* const* argv,
                         const std::vector<TCLAP::Arg*>& cmd_line_args) {
    TCLAP::CmdLine cmd_line(model_description);
    Json::Reader reader;

    if (!reader.parse(DEFAULT_CONFIG, *root_)) {
        throw std::runtime_error(std::string("Failed to parse default configuration\n") +
                                 reader.getFormattedErrorMessages());
    }

    TCLAP::ValueArg<std::string> config_arg("c", "config", "Warped configuration file",
                                            false, config_file_name_, "file", cmd_line);
    TCLAP::ValueArg<unsigned int> max_sim_time_arg("t", "max-sim-time",
                                                   "specify a simulation end time",
                                                   false, max_sim_time_, "time", cmd_line);
    addParameters(cmd_line, cmd_line_args);
    cmd_line.parse(argc, argv);

    max_sim_time_ = max_sim_time_arg.getValue();
    config_file_name_ = config_arg.getValue();

    if (!config_file_name_.empty()) {
        auto root2 = parseJsonFile(config_file_name_);
        update(*root_, root2);
    }

}

std::unique_ptr<EventDispatcher> Configuration::make_dispatcher() {
    if ((*root_)["simulation-type"] != "sequential") {
        //TODO: return a TimeWarpEventDispatcer
    }
    return std::unique_ptr<EventDispatcher>{new SequentialEventDispatcher{max_sim_time_}};
}

} // namespace warped