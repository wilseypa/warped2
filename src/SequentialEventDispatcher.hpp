#ifndef SEQUENTIAL_EVENT_DISPATCHER_HPP
#define SEQUENTIAL_EVENT_DISPATCHER_HPP

#include <memory>
#include <unordered_map>

#include "EventDispatcher.hpp"

namespace warped {

class EventStatistics;
class SimulationObject;

// This class is an EventDispatcher that runs on a single thread and process.
class SequentialEventDispatcher : public EventDispatcher {
public:
    SequentialEventDispatcher(unsigned int max_sim_time, std::unique_ptr<EventStatistics> stats);

    void startSimulation(const std::vector<std::vector<SimulationObject*>>& objects);

    unsigned int getSimulationTime(SimulationObject* object);

    FileStream& getFileStream(SimulationObject* object, const std::string& filename,
        std::ios_base::openmode mode);

private:
    unsigned int current_sim_time_ = 0;

    std::unique_ptr<EventStatistics> stats_;

    std::unordered_map<std::string, std::unique_ptr<FileStream, FileStreamDeleter>>
        file_stream_by_filename_;

    std::unordered_map<std::string, SimulationObject*> object_by_filename_;

};

} // namespace warped

#endif
