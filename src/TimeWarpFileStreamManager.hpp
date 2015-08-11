#ifndef TIME_WARP_SIMULATION_OBJECT_FILESTREAM
#define TIME_WARP_SIMULATION_OBJECT_FILESTREAM

#include <sstream>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

#include "TimeWarpFileStream.hpp"
#include "LogicalProcess.hpp"

/* This class manages TimeWarpFileStreams for each lp. Only one lp of this class is needed
 * per node and is used only by TimeWarpEventDispatcher.
 * Each lp has a vector of TimeWarpFileStreams which are associated with it.
 */

namespace warped {

class TimeWarpFileStream;

class TimeWarpFileStreamManager {
public:
    // Create FileStream vector and lock for each lp
    void initialize(unsigned int num_local_lps);

    // Removes any output request after rollback time for the specified lp
    void rollback(std::shared_ptr<Event> rollback_event, unsigned int local_lp_id);

    // Commits any output request before or equal to the gvt for the specified lp
    void fossilCollect(unsigned int gvt, unsigned int local_lp_id);

    // Retrieves the specified TimeWarpFileStream by looking at filename. If it has not been
    // created, then one will be created and returned.
    TimeWarpFileStream* getFileStream(const std::string& filename, std::ios_base::openmode mode,
        unsigned int local_lp_id, std::shared_ptr<Event> this_event);

    // Number of TimeWarpFileStreams associated with the specified lp
    std::size_t size(unsigned int local_lp_id);

private:
    // An array of TimeWarpFileStream vectors, one for each lp.
    std::unique_ptr<std::vector<std::unique_ptr<TimeWarpFileStream, FileStreamDeleter>> []>
        file_streams_;

    std::unique_ptr<std::mutex []> file_streams_lock_;

    // Map to get TimeWarpFileStream* given a filename.
    std::unordered_map<std::string, TimeWarpFileStream*> file_stream_by_filename_;

    unsigned int num_local_lps_;
};

} // namespace warped

#endif
