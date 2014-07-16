#include "TimeWarpFileStreamManager.hpp"

#include <stdexcept>

#include "TimeWarpFileStream.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpFileStreamManager::rollback(unsigned int rollback_time, unsigned int local_object_id) {
    file_streams_lock_[local_object_id].lock();
    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->removeOutputRequestsAfterOrAt(rollback_time);
    }
    file_streams_lock_[local_object_id].unlock();
}

void TimeWarpFileStreamManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    file_streams_lock_[local_object_id].lock();
    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->commitOutputRequestsBeforeOrAt(gvt);
    }
    file_streams_lock_[local_object_id].unlock();
}

void TimeWarpFileStreamManager::fossilCollectAll(unsigned int gvt) {
    for (unsigned int i = 0; i < file_streams_.get()->size(); i++) {
        fossilCollect(gvt, i);
    }
}

void TimeWarpFileStreamManager::setObjectCurrentTime(unsigned int current_time,
    unsigned int local_object_id) {
    file_streams_lock_[local_object_id].lock();
    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->setCurrentTime(current_time);
    }
    file_streams_lock_[local_object_id].unlock();
}

void TimeWarpFileStreamManager::initialize(unsigned int num_local_objects) {
    file_streams_ = make_unique<std::vector<std::unique_ptr<TimeWarpFileStream,
        FileStreamDeleter>> []>(num_local_objects);
    file_streams_lock_ = make_unique<std::mutex []>(num_local_objects);
}

TimeWarpFileStream* TimeWarpFileStreamManager::getFileStream(const std::string& filename,
    std::ios_base::openmode mode, unsigned int local_object_id) {

    TimeWarpFileStream* retval;

    file_streams_lock_[local_object_id].lock();
    if (file_stream_by_filename_.count(filename) == 0) {
        std::unique_ptr<TimeWarpFileStream, FileStreamDeleter> twfs(new TimeWarpFileStream(
            filename, mode), FileStreamDeleter());
        file_stream_by_filename_[filename] = twfs.get();
        file_streams_[local_object_id].push_back(std::move(twfs));
        retval = file_streams_[local_object_id].back().get();
    } else {
        auto& this_objects_filestreams = file_streams_[local_object_id];
        bool is_this_objects = false;
        auto twfs = file_stream_by_filename_[filename];
        for (auto& stream: this_objects_filestreams) {
            if (stream.get() == twfs) {
                is_this_objects = true;
                break;
            }
        }
        if (is_this_objects) {
            retval = twfs;
        } else {
            throw std::runtime_error(std::string(
                "A filestream can only be used by a single object"));
        }
    }

    file_streams_lock_[local_object_id].unlock();
    return retval;
}

std::size_t TimeWarpFileStreamManager::size(unsigned int local_object_id) {
    return file_streams_[local_object_id].size();
}

} // namespace warped

