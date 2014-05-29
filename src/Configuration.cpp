#include "Configuration.hpp"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "tclap/CmdLine.h"
#include "json/json.h"

#include "CommandLineConfiguration.hpp"
#include "RoundRobinPartitioner.hpp"
#include "SequentialEventDispatcher.hpp"

namespace {
const static std::string DEFAULT_CONFIG = R"x({

// If max-sim-time > 0, the simualtion will halt once the time has been reached
"max-sim-time":0,

// Valid options are "sequential" and "time-warp"
"simulation-type": "sequential",

// Valid options are "default" and "round-robin". "default" will use user
// provided provided if given, else "round-robin".
"partitioning": "default"

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
    : config_file_name_(config_file_name), max_sim_time_(max_sim_time), root_(nullptr)
{ readUserConfig(); }

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

void Configuration::readUserConfig() {
    if (!config_file_name_.empty()) {
        auto root2 = parseJsonFile(config_file_name_);
        update(*root_, root2);
    }
}

void Configuration::init(const std::string& model_description, int argc, const char* const* argv,
                         const std::vector<TCLAP::Arg*>& cmd_line_args) {
    Json::Reader reader;

    if (!reader.parse(DEFAULT_CONFIG, *root_)) {
        throw std::runtime_error(std::string("Failed to parse default configuration\n") +
                                 reader.getFormattedErrorMessages());
    }

    // Update the JSON root with any command line args given
    bool should_dump;
    CommandLineConfiguration clc(*root_);
    std::tie(should_dump, config_file_name_) = clc.parse(model_description, argc, argv, cmd_line_args);

    readUserConfig();

    if (should_dump) {
        std::cout << *root_ << std::endl;
        std::exit(0);
    }
}

std::unique_ptr<EventDispatcher> Configuration::makeDispatcher() {
    if ((*root_)["simulation-type"].asString() != "sequential") {
        //TODO: Create, configure, and return a TimeWarpEventDispatcher
    }

    return std::unique_ptr<EventDispatcher> {new SequentialEventDispatcher{max_sim_time_}};
}

std::unique_ptr<Partitioner> Configuration::makePartitioner() {
    auto partitioner_type = (*root_)["partitioning"].asString();
    if (partitioner_type == "default" || partitioner_type == "round-robin") {
        return std::unique_ptr<Partitioner> {new RoundRobinPartitioner{}};
    }
    throw std::runtime_error(std::string("Invalid partitioning type: ") + partitioner_type);
}

std::unique_ptr<Partitioner>
Configuration::makePartitioner(std::unique_ptr<Partitioner> user_partitioner) {
    const auto& partitioner_type = (*root_)["partitioning"];
    if (partitioner_type == "default") {
        return std::move(user_partitioner);
    }
    return makePartitioner();
}

} // namespace warped