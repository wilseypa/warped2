#include "Simulation.hpp"

#include <memory>
#include <vector>
#include <tuple>

#include "Configuration.hpp"
#include "EventDispatcher.hpp"
#include "Partitioner.hpp"
#include "SimulationObject.hpp"
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

void Simulation::simulate(const std::vector<SimulationObject*>& objects) {

    auto comm_manager = config_.makeCommunicationManager();

    unsigned int num_partitions = comm_manager->initialize();
    auto partitioned_objects = config_.makePartitioner()->partition(objects, num_partitions);
    comm_manager->initializeObjectMap(partitioned_objects);

    unsigned int num_schedulers = num_partitions;
    auto local_partitioner = config_.makeLocalPartitioner(comm_manager->getID(), num_schedulers);
    auto local_partitions =
        local_partitioner->partition(partitioned_objects[comm_manager->getID()], num_schedulers);

    event_dispatcher_ = config_.makeDispatcher(comm_manager);
    event_dispatcher_->startSimulation(local_partitions);

    comm_manager->finalize();
}

void Simulation::simulate(const std::vector<SimulationObject*>& objects,
    std::unique_ptr<Partitioner> partitioner) {

    auto comm_manager = config_.makeCommunicationManager();

    unsigned int num_partitions = comm_manager->initialize();
    auto partitioned_objects =
        config_.makePartitioner(std::move(partitioner))->partition(objects, num_partitions);
    comm_manager->initializeObjectMap(partitioned_objects);

    unsigned int num_schedulers = num_partitions;
    auto local_partitioner = config_.makeLocalPartitioner(comm_manager->getID(), num_schedulers);
    auto local_partitions =
        local_partitioner->partition(partitioned_objects[comm_manager->getID()], num_schedulers);

    event_dispatcher_ = config_.makeDispatcher(comm_manager);
    event_dispatcher_->startSimulation(local_partitions);

    comm_manager->finalize();
}

FileStream& Simulation::getFileStream(SimulationObject* object, const std::string& filename,
    std::ios_base::openmode mode, std::shared_ptr<Event> this_event) {

    return event_dispatcher_->getFileStream(object, filename, mode, this_event);
}

} // namepsace warped
