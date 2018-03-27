#include "Simulation.hpp"

#include <memory>
#include <vector>
#include <tuple>
#include <algorithm>

#include "Configuration.hpp"
#include "EventDispatcher.hpp"
#include "Partitioner.hpp"
#include "LogicalProcess.hpp"
#include "TimeWarpMPICommunicationManager.hpp"

extern "C" {
    void warped_is_present(void) {
        return;
    }
}

namespace warped {

std::unique_ptr<EventDispatcher> Simulation::event_dispatcher_;

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv,
                       const std::vector<TCLAP::Arg*>& cmd_line_args)
    : config_(model_description, argc, argv, cmd_line_args) {}

Simulation::Simulation(const std::string& model_description, int argc, const char* const* argv)
    : config_(model_description, argc, argv) {}

Simulation::Simulation(const std::string& config_file_name, unsigned int max_sim_time)
    : config_(config_file_name, max_sim_time) {}

void Simulation::simulate(const std::vector<LogicalProcess*>& lps) {
    check(lps);

    auto comm_manager = config_.makeCommunicationManager();

    unsigned int num_nodes = comm_manager->initialize();
    auto partitioner = config_.makePartitioner();
    auto partitioned_lps = partitioner->interNodePartition(lps, num_nodes);

    comm_manager->initializeLPMap(partitioned_lps);

    comm_manager->waitForAllProcesses();

    auto local_partitions = 
                partitioner->intraNodePartition(partitioned_lps[comm_manager->getID()]);

    event_dispatcher_ = config_.makeDispatcher(comm_manager);
    event_dispatcher_->startSimulation(local_partitions);

    comm_manager->finalize();
}

void Simulation::simulate(const std::vector<LogicalProcess*>& lps,
    std::unique_ptr<Partitioner> partitioner) {

    check(lps);
    auto comm_manager = config_.makeCommunicationManager();

    unsigned int num_nodes = comm_manager->initialize();
    auto custom_partitioner = config_.makePartitioner(std::move(partitioner));
    auto partitioned_lps = custom_partitioner->interNodePartition(lps, num_nodes);
    comm_manager->initializeLPMap(partitioned_lps);

    comm_manager->waitForAllProcesses();

    auto local_partitions =
            custom_partitioner->intraNodePartition(partitioned_lps[comm_manager->getID()]);

    event_dispatcher_ = config_.makeDispatcher(comm_manager);
    event_dispatcher_->startSimulation(local_partitions);

    comm_manager->finalize();
}

void Simulation::check(const std::vector<LogicalProcess*>& lps) {
   if(std::adjacent_find(lps.begin(), lps.end(), equalNames) != lps.end())
      throw std::runtime_error(std::string("Two LogicalProcess with the same name."));
}

FileStream& Simulation::getFileStream(LogicalProcess* lp, const std::string& filename,
    std::ios_base::openmode mode, std::shared_ptr<Event> this_event) {

    return event_dispatcher_->getFileStream(lp, filename, mode, this_event);
}

} // namepsace warped
