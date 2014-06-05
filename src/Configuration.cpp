#include "Configuration.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "json/json.h"

#include "AggregateEventStatistics.hpp"
#include "CommandLineConfiguration.hpp"
#include "EventDispatcher.hpp"
#include "EventStatistics.hpp"
#include "IndividualEventStatistics.hpp"
#include "NullEventStatistics.hpp"
#include "Partitioner.hpp"
#include "ProfileGuidedPartitioner.hpp"
#include "RoundRobinPartitioner.hpp"
#include "SequentialEventDispatcher.hpp"
#include "TimeWarpEventDispatcher.hpp"
#include "utility/memory.hpp"

namespace {
const static std::string DEFAULT_CONFIG = R"x({

// If max-sim-time > 0, the simulation will halt once the time has been reached
"max-sim-time": 0,

// Valid options are "sequential" and "time-warp"
"simulation-type": "sequential",

"statistics": {
    // Valid options are "none", "json", "csv", "graphviz", and "metis".
    // "json" and "csv" are individual statistics, others are aggregate.
    "type": "none",
    // If statistics-type is not "none", save the output in this file
    "file": "statistics.out"
},

"partitioning": {
    // Valid options are "default", "round-robin" and "profile-guided".
    // "default" will use user provided partitioning if given, else
    // "round-robin".
    "type": "default",
    // The path to the statistics file that was created from a previous run.
    // Only used if "partitioning-type" is "provided-guided".
    "file": "statistics.out"
}

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
    : config_file_name_(""), max_sim_time_(0), root_(make_unique<Json::Value>())
{ init(model_description, argc, argv, cmd_line_args); }

Configuration::Configuration(const std::string& model_description, int argc,
                             const char* const* argv)
    : config_file_name_(""), max_sim_time_(0), root_(make_unique<Json::Value>())
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
    if ((*root_)["max-sim-time"].isUInt()) {
        max_sim_time_ = (*root_)["max-sim-time"].asUInt();
    }

    if ((*root_)["simulation-type"].asString() == "time-warp") {
        //TODO: Create, configure, and return a TimeWarpEventDispatcher
        // This is just a rough idea of how the dispatcher could be configured
        // if ((*root_)["ltsf-queue"].asString() == "ladder-queue" {
        //     std::unique_ptr<LTSFQueue> queue = make_unique<LadderQueue>();
        // }
        // return make_unique<TimeWarpEventDispatcher>(max_sim_time_, std::move(queue));
    }

    // Return a SequentialEventDispatcher by default
    std::unique_ptr<EventStatistics> stats;
    auto statistics_type = (*root_)["statistics"]["type"].asString();
    auto statistics_file = (*root_)["statistics"]["file"].asString();
    if (statistics_type == "json") {
        auto type = IndividualEventStatistics::OutputType::Json;
        stats = make_unique<IndividualEventStatistics>(statistics_file, type);
    } else if (statistics_type == "csv") {
        auto type = IndividualEventStatistics::OutputType::Csv;
        stats = make_unique<IndividualEventStatistics>(statistics_file, type);
    } else if (statistics_type == "graphviz") {
        auto type = AggregateEventStatistics::OutputType::Graphviz;
        stats = make_unique<AggregateEventStatistics>(statistics_file, type);
    } else if (statistics_type == "metis") {
        auto type = AggregateEventStatistics::OutputType::Metis;
        stats = make_unique<AggregateEventStatistics>(statistics_file, type);
    } else {
        stats = make_unique<NullEventStatistics>();
    }
    return make_unique<SequentialEventDispatcher>(max_sim_time_, std::move(stats));
}

std::unique_ptr<Partitioner> Configuration::makePartitioner() {
    auto partitioner_type = (*root_)["partitioning"]["type"].asString();
    if (partitioner_type == "default" || partitioner_type == "round-robin") {
        return make_unique<RoundRobinPartitioner>();
    } else if (partitioner_type == "profile-guided") {
        auto filename = (*root_)["partitioning"]["file"].asString();
        std::ifstream input(filename);
        if (!input.is_open()) {
            throw std::runtime_error(std::string("Could not open statistics file ") + filename);
        }
        return make_unique<ProfileGuidedPartitioner>(input);
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