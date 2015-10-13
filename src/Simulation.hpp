#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <string>
#include <vector>

#include "Configuration.hpp"
#include "FileStream.hpp"
#include "LogicalProcess.hpp"

namespace TCLAP { class Arg; }

namespace warped {
class Event;

// The main entry point into the simulation.
//
// This class sets up the simulation and handles configuration and optionally
// command line processing.
class Simulation {
public:
    // This constructor will take care of processing any command line
    // arguments automatically.
    //
    // The model_description is a short description of the model that will be
    // displayed on the command line.
    Simulation(const std::string& model_description, int argc, const char* const* argv);

    // This is the same as the above constructor, but takes a vector of extra
    // command line parameters. That the model defines.
    Simulation(const std::string& model_description, int argc, const char* const* argv,
               const std::vector<TCLAP::Arg*>& cmd_line_args);

    // This constructor doesn't do any command line processing.
    Simulation(const std::string& config_file_name, unsigned int max_time);

    // This will start the simulation, using the lps passed in.
    //
    // If a Time Warp simulation is run, the lps will be automatically
    // partitioned.
    void simulate(const std::vector<LogicalProcess*>& lps);

    // A model can perform its own partitioning by calling this overload of simulate().
    //
    // The user supplied partition may not be used if a specific partitioner
    // is configured.
    void simulate(const std::vector<LogicalProcess*>& lps,
                  std::unique_ptr<Partitioner> partitioner);

    static FileStream& getFileStream(LogicalProcess* lp, const std::string& filename,
        std::ios_base::openmode mode, std::shared_ptr<Event> this_event);

private:
    void inline check(const std::vector<LogicalProcess*>& lps);
    Configuration config_;
    static std::unique_ptr<EventDispatcher> event_dispatcher_;
};

bool inline equalNames(const LogicalProcess* lproc1, const LogicalProcess* lproc2) {
   return lproc1->name_ == lproc2->name_;
}
} // namespace warped

#endif

