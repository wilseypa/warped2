#ifndef TIME_WARP_SIMULATION_OBJECT_FILESTREAM
#define TIME_WARP_SIMULATION_OBJECT_FILESTREAM

#include <sstream>
#include <memory>
#include <vector>
#include <mutex>

#include "TimeWarpFileStream.hpp"
#include "SimulationObject.hpp"

namespace warped {

class TimeWarpFileStream;

class TimeWarpFileStreamManager {
public:
    void initialize(unsigned int num_local_objects);

    void insert(std::shared_ptr<TimeWarpFileStream> twfstream, unsigned int local_object_id);

    void rollback(unsigned int rollback_time, unsigned int local_object_id);

    void fossilCollect(unsigned int gvt, unsigned int local_object_id);

    void fossilCollectAll(unsigned int gvt);

    void setObjectCurrentTime(unsigned int current_time, unsigned int local_object_id);

    bool objectHasFileStream(std::shared_ptr<TimeWarpFileStream> twfstream,
        unsigned int local_object_id);

    std::size_t size(unsigned int local_object_id);

private:
    // An array of TimeWarpFileStream vectors, one for each object.
    std::unique_ptr<std::vector<std::shared_ptr<TimeWarpFileStream>> []> file_streams_;

    std::unique_ptr<std::mutex []> file_streams_lock_;
};

} // namespace warped

#endif
