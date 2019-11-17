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
#include <iostream>

#include "json/json.h"

#include "config.h"
#include "AggregateEventStatistics.hpp"
#include "CommandLineConfiguration.hpp"
#include "EventDispatcher.hpp"
#include "EventStatistics.hpp"
#include "IndividualEventStatistics.hpp"
#include "NullEventStatistics.hpp"
#include "Partitioner.hpp"
#include "STLLTSFQueue.hpp"
#include "ProfileGuidedPartitioner.hpp"
#include "RoundRobinPartitioner.hpp"
#include "SequentialEventDispatcher.hpp"
#include "TimeWarpAsynchronousGVTManager.hpp"
#include "TimeWarpSynchronousGVTManager.hpp"
#include "TimeWarpEventDispatcher.hpp"
#include "utility/memory.hpp"
#include "TimeWarpMPICommunicationManager.hpp"
#include "TimeWarpPeriodicStateManager.hpp"
#include "TimeWarpAggressiveOutputManager.hpp"
#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpEventSet.hpp"
#include "TimeWarpTerminationManager.hpp"
#include "TimeWarpStatistics.hpp"

namespace {
const static std::string DEFAULT_CONFIG = R"x({

// If max-sim-time > 0, the simulation will halt once the time has been reached
"max-sim-time": 0,

// Valid options are "sequential" and "time-warp"
"simulation-type": "time-warp",

"statistics": {
    // Valid options are "none", "json", "csv", "graphviz", and "metis".
    // "json" and "csv" are individual statistics, others are aggregate.
    "type": "none",
    // If statistics-type is not "none", save the output in this file
    "file": "statistics.out"
},

"time-warp" : {
    "gvt-calculation": {
        // GVT calculation period
        "period": 1000,
        // "synchronous" or "asynchronous"
        "method": "asynchronous"
    },

    "state-saving": {
        // State saving type, default "periodic"
        "type": "periodic",
        // State saving period
        "period": 10
    },

    // Cancellation type, default "aggressive"
    "cancellation": "aggressive",

    // Number of Worker Threads
    "worker-threads": 3,

    // Number of Schedule Queues
    "scheduler-count": 1,

    // LP Migration valid options are "on" and "off"
    "lp-migration": "off",

    // Name of file to dump stats, "none" to disable
    "statistics-file" : "none",

    // Name of the config output file
    "config-output-file" : "none",

    "communication" : {
        // Max message size
        "max-msg-size" : 512,
        // Max message aggregation count
        "max-aggregate" : 5
    }
},

"partitioning": {
    // Valid options are "default", "round-robin" and "profile-guided".
    // "default" will use user provided partitioning if given, else
    // "round-robin".
    "type": "default",

    // Number of LPs to partition at a time.
    // Only used if "partitioning-type" is "round-robin".
    "blocksize": 0,

    // The path to the statistics file that was created from a previous run.
    // Only used if "partitioning-type" is "profile-guided".
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

bool Configuration::checkTimeWarpConfigs(uint64_t local_config_id, uint64_t *all_config_ids,
    std::shared_ptr<TimeWarpCommunicationManager> comm_manager) {

    comm_manager->gatherUint64(&local_config_id, all_config_ids);
    if (comm_manager->getID() == 0) {
        for (unsigned int i = 0; i < comm_manager->getNumProcesses(); i++) {
            if (all_config_ids[i] != local_config_id) {
                return false;
            }
        }
    }
    return true;
}

std::unique_ptr<EventDispatcher>
Configuration::makeDispatcher(std::shared_ptr<TimeWarpCommunicationManager> comm_manager) {
    uint64_t local_config_id;
    uint64_t *all_config_ids = new uint64_t[comm_manager->getNumProcesses()];
    std::string invalid_string;

    // MAX SIMULATION_TIME
    if ((*root_)["max-sim-time"].isUInt()) {
        max_sim_time_ = (*root_)["max-sim-time"].asUInt();
    }
    if (!checkTimeWarpConfigs(max_sim_time_, all_config_ids, comm_manager)) {
        invalid_string += std::string("\tMaximum simulation time\n");
    }

    auto simulation_type = (*root_)["simulation-type"].asString();
    if (simulation_type == "time-warp") {
        local_config_id = 1;
        if(!checkTimeWarpConfigs(local_config_id, all_config_ids, comm_manager)) {
            invalid_string += std::string("\tSimulation type\n");
        }

        std::unique_ptr<TimeWarpEventSet> event_set = make_unique<TimeWarpEventSet>();

// If we only want one thread per queue, then we tie the amount of worker treads to the amount of schedule queues
// Number of worker threads is defined after the number of schedule queues if defined below
#if !defined (ONE_THREAD_PER_LTSF)
        // WORKER THREADS
        int num_worker_threads = (*root_)["time-warp"]["worker-threads"].asInt();
        if (!checkTimeWarpConfigs(num_worker_threads, all_config_ids, comm_manager)) {
            invalid_string += std::string("\tNumber of worker threads\n");
        }
#endif

        // SCHEDULE QUEUES
        int num_schedulers = (*root_)["time-warp"]["scheduler-count"].asInt();
        if (!checkTimeWarpConfigs(num_schedulers, all_config_ids, comm_manager)) {
            invalid_string += std::string("\tNumber of schedule queues\n");
        }

// If we disable schedule queue locks, then we need to make sure that there are 
// exactly one worker thread per schedule queue
#if defined (ONE_THREAD_PER_LTSF)
        int num_worker_threads = num_schedulers;
        assert(num_worker_threads%num_schedulers == 0);
#endif

        // LP MIGRATION
        auto lp_migration_status = (*root_)["time-warp"]["lp-migration"].asString();
        if (lp_migration_status == "off") {
            local_config_id = 1;
            if(!checkTimeWarpConfigs(local_config_id, all_config_ids, comm_manager)) {
                invalid_string += std::string("\tLP Migration\n");
            }
        }
        bool is_lp_migration_on = (lp_migration_status == "on") ? true : false;

        // STATE MANAGER
        std::unique_ptr<TimeWarpStateManager> state_manager;
        int state_period = 0;
        auto state_saving_type = (*root_)["time-warp"]["state-saving"]["type"].asString();
        if (state_saving_type == "periodic") {
            // local_config_id == 0 for periodic
            local_config_id = 0;
            if (!checkTimeWarpConfigs(local_config_id, all_config_ids, comm_manager)) {
                invalid_string += std::string("\tState-saving type\n");
            }

            state_period = (*root_)["time-warp"]["state-saving"]["period"].asInt();
            if (!checkTimeWarpConfigs(state_period, all_config_ids, comm_manager)) {
                invalid_string += std::string("\tState saving period\n");
            }

            state_manager = make_unique<TimeWarpPeriodicStateManager>(state_period);
        }

        // OUTPUT MANAGER
        std::unique_ptr<TimeWarpOutputManager> output_manager;
        auto cancellation_type = (*root_)["time-warp"]["cancellation"].asString();
        if (cancellation_type == "aggressive") {
            // local_config_id == 0 for aggressive
            local_config_id = 0;
            if (!checkTimeWarpConfigs(local_config_id, all_config_ids, comm_manager)) {
                invalid_string += std::string("\tCancellation type\n");
            }
            output_manager = make_unique<TimeWarpAggressiveOutputManager>();
        }

        // GVT
        unsigned int gvt_period = (*root_)["time-warp"]["gvt-calculation"]["period"].asUInt();
        if (!checkTimeWarpConfigs(gvt_period, all_config_ids, comm_manager)) {
            invalid_string += std::string("\tGVT period\n");
        }

        std::unique_ptr<TimeWarpGVTManager> gvt_manager;
        auto gvt_method = (*root_)["time-warp"]["gvt-calculation"]["method"].asString();
        if (gvt_method == "synchronous") {
            gvt_manager =
                make_unique<TimeWarpSynchronousGVTManager>(comm_manager, gvt_period, num_worker_threads);
        } else if (gvt_method == "asynchronous") {
            gvt_manager =
                make_unique<TimeWarpAsynchronousGVTManager>(comm_manager, gvt_period, num_worker_threads);
        } else {
            invalid_string += std::string("\tInvalid GVT Method\n");
        }

        // TERMINATION
        std::unique_ptr<TimeWarpTerminationManager> termination_manager =
            make_unique<TimeWarpTerminationManager>(comm_manager);

        // FILE STREAMS
        std::unique_ptr<TimeWarpFileStreamManager> twfs_manager =
            make_unique<TimeWarpFileStreamManager>();

        // STATISTICS
        auto stats_file = (*root_)["time-warp"]["statistics-file"].asString();
        std::unique_ptr<TimeWarpStatistics> tw_stats =
            make_unique<TimeWarpStatistics>(comm_manager, stats_file);

        if (!invalid_string.empty()) {
            throw std::runtime_error(std::string("Configuration files do not match, \
check the following configurations:\n") + invalid_string);
        }

        delete [] all_config_ids;

        if (comm_manager->getID() == 0) {
            std::cout << "Type of build:             ";
#ifdef NDEBUG
            std::cout << "Optimized\n";
#else
            std::cout << "Debug\n";
#endif

            std::cout << "Simulation type:           " << simulation_type << "\n"
                      << "Number of processes:       " << comm_manager->getNumProcesses() << "\n"
                      << "Number of worker threads:  " << num_worker_threads << "\n"
                      << "Number of Schedule queues: " << num_schedulers << "\n";

            std::cout << "Type of Schedule queue:    ";
#ifdef SORTED_LADDER_QUEUE
            std::cout << "Sorted Ladder Queue\n";
#elif defined(PARTIALLY_SORTED_LADDER_QUEUE)
            std::cout << "Partially Sorted Ladder Queue\n";
#elif defined(SPLAY_TREE)
            std::cout << "Splay Tree\n";
#elif defined(CIRCULAR_QUEUE)
            std::cout << "Circular Queue\n";
#else
            std::cout << "MultiSet\n";
#endif

            std::cout << "Type of Scheduler lock:    ";
#ifdef SCHEDULE_QUEUE_SPINLOCKS
            std::cout << "SpinLock\n";
#else
            std::cout << "Mutex\n";
#endif

            std::cout << "LP Migration:              " << lp_migration_status << "\n"
                      << "State-saving type:         " << state_saving_type << "\n";
            if (state_saving_type == "periodic")
            std::cout << "State-saving period:       " << state_period << " events" << "\n";
            std::cout << "Cancellation type:         " << cancellation_type << "\n"
                      << "GVT Period:                " << gvt_period << " ms" << "\n"
                      << "Max simulation time:       " \
                        << (max_sim_time_ ? std::to_string(max_sim_time_) : "infinity") << std::endl << std::endl;

            auto output_config_file = (*root_)["time-warp"]["config-output-file"].asString();
            if (output_config_file != "none") {
                std::ofstream ofs(output_config_file);
                ofs << *root_ << std::endl;
                ofs.close();
            }
        }

        return make_unique<TimeWarpEventDispatcher>(max_sim_time_,
            num_worker_threads, is_lp_migration_on, comm_manager,
            std::move(event_set), std::move(gvt_manager), std::move(state_manager),
            std::move(output_manager), std::move(twfs_manager),
            std::move(termination_manager), std::move(tw_stats));
    }

    if (comm_manager->getNumProcesses() > 1) {
            throw std::runtime_error("Sequential simulation must be single process\n");
    }

    std::cout << "Simulation type: Sequential" << std::endl;

    // Return a SequentialEventDispatcher by default
    std::unique_ptr<EventStatistics> stats;
    auto statistics_type = (*root_)["statistics"]["type"].asString();
    auto statistics_file = (*root_)["statistics"]["file"].asString();
    std::cout << "Statistics type: " << statistics_type << std::endl;
    std::cout << "Statistics file: " << statistics_file << std::endl;
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
        auto blocksize = (*root_)["partitioning"]["blocksize"].asUInt();
        return make_unique<RoundRobinPartitioner>(blocksize);
    } else if (partitioner_type == "profile-guided") {
        auto filename = (*root_)["partitioning"]["file"].asString();
        return make_unique<ProfileGuidedPartitioner>(filename, "partition");
    }
    throw std::runtime_error(std::string("Invalid partitioning type: ") + partitioner_type);
}

std::unique_ptr<Partitioner>
Configuration::makePartitioner(std::unique_ptr<Partitioner> user_partitioner) {
    const auto& partitioner_type = (*root_)["partitioning"]["type"];
    if (partitioner_type == "default") {
        return user_partitioner;
    }
    return makePartitioner();
}

std::unique_ptr<Partitioner> Configuration::makeLocalPartitioner(unsigned int node_id,
    unsigned int& num_schedulers) {

    unsigned int num_partitions = num_schedulers;
    num_schedulers = (*root_)["time-warp"]["scheduler-count"].asUInt();

    if (num_partitions == 1)
        return makePartitioner();

    auto partitioner_type = (*root_)["partitioning"]["type"].asString();
    if (partitioner_type == "default" || partitioner_type == "round-robin") {
        auto blocksize = (*root_)["partitioning"]["blocksize"].asUInt();
        return make_unique<RoundRobinPartitioner>(blocksize);
    } else if (partitioner_type == "profile-guided") {
        return make_unique<ProfileGuidedPartitioner>("partitions/partition"+std::to_string(node_id)+".out",
            "partition"+std::to_string(node_id));
    }
    throw std::runtime_error(std::string("Invalid partitioning type: ") + partitioner_type);
}

std::shared_ptr<TimeWarpCommunicationManager> Configuration::makeCommunicationManager() {
    unsigned int max_msg_size = (*root_)["time-warp"]["communication"]["max-msg-size"].asUInt();
    unsigned int max_aggregate = (*root_)["time-warp"]["communication"]["max-aggregate"].asUInt();

    return std::make_shared<TimeWarpMPICommunicationManager>(max_msg_size, max_aggregate);
}

} // namespace warped

