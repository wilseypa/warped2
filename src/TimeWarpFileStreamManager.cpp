#include "TimeWarpFileStreamManager.hpp"

#include <stdexcept>

#include "TimeWarpFileStream.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpFileStreamManager::rollback(std::shared_ptr<Event> rollback_event, unsigned int local_lp_id) {
    file_streams_lock_[local_lp_id].lock();
    auto& this_lps_streams = file_streams_[local_lp_id];
    for (auto& stream: this_lps_streams) {
        stream->removeOutputRequestsAfter(rollback_event);
        stream->setCurrentEvent(rollback_event);
    }
    file_streams_lock_[local_lp_id].unlock();
}

void TimeWarpFileStreamManager::fossilCollect(unsigned int gvt, unsigned int local_lp_id) {
    file_streams_lock_[local_lp_id].lock();
    auto& this_lps_streams = file_streams_[local_lp_id];
    for (auto& stream: this_lps_streams) {
        stream->commitOutputRequestsBefore(gvt);
    }
    file_streams_lock_[local_lp_id].unlock();
}

void TimeWarpFileStreamManager::initialize(unsigned int num_local_lps) {
    file_streams_ = make_unique<std::vector<std::unique_ptr<TimeWarpFileStream,
        FileStreamDeleter>> []>(num_local_lps);
    file_streams_lock_ = make_unique<std::mutex []>(num_local_lps);
    num_local_lps_ = num_local_lps;
}

TimeWarpFileStream* TimeWarpFileStreamManager::getFileStream(const std::string& filename,
    std::ios_base::openmode mode, unsigned int local_lp_id, std::shared_ptr<Event> this_event) {

    TimeWarpFileStream* retval;

    file_streams_lock_[local_lp_id].lock();
    if (file_stream_by_filename_.count(filename) == 0) {
        std::unique_ptr<TimeWarpFileStream, FileStreamDeleter> twfs(new TimeWarpFileStream(
            filename, mode), FileStreamDeleter());
        file_stream_by_filename_[filename] = twfs.get();
        file_streams_[local_lp_id].push_back(std::move(twfs));
        retval = file_streams_[local_lp_id].back().get();
    } else {
        auto& this_lps_filestreams = file_streams_[local_lp_id];
        bool is_this_lps = false;
        auto twfs = file_stream_by_filename_[filename];
        for (auto& stream: this_lps_filestreams) {
            if (stream.get() == twfs) {
                is_this_lps = true;
                break;
            }
        }
        if (is_this_lps) {
            retval = twfs;
        } else {
            throw std::runtime_error(std::string(
                "A filestream can only be used by a single lp"));
        }
    }

    retval->setCurrentEvent(this_event);

    file_streams_lock_[local_lp_id].unlock();
    return retval;
}

std::size_t TimeWarpFileStreamManager::size(unsigned int local_lp_id) {
    return file_streams_[local_lp_id].size();
}

} // namespace warped

