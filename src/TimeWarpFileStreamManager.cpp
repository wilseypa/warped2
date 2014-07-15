#include "TimeWarpFileStreamManager.hpp"
#include "TimeWarpFileStream.hpp"
#include "utility/memory.hpp"

namespace warped {

void TimeWarpFileStreamManager::insert(std::shared_ptr<TimeWarpFileStream> twfstream,
    unsigned int local_object_id) {

    file_streams_[local_object_id].push_back(twfstream);
}

void TimeWarpFileStreamManager::rollback(unsigned int rollback_time, unsigned int local_object_id) {
    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->removeOutputRequestsBeforeOrAt(rollback_time);
    }
}

void TimeWarpFileStreamManager::fossilCollect(unsigned int gvt, unsigned int local_object_id) {
    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->commitOutputRequestsAfterOrAt(gvt);
    }
}

void TimeWarpFileStreamManager::fossilCollectAll(unsigned int gvt) {
    for (unsigned int i = 0; i < file_streams_.get()->size(); i++) {
        fossilCollect(gvt, i);
    }
}

void TimeWarpFileStreamManager::setObjectCurrentTime(unsigned int current_time,
    unsigned int local_object_id) {

    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        stream->setCurrentTime(current_time);
    }
}

void TimeWarpFileStreamManager::initialize(unsigned int num_local_objects) {
    file_streams_ = make_unique<std::vector<std::shared_ptr<TimeWarpFileStream>> []>
        (num_local_objects);
}

bool TimeWarpFileStreamManager::objectHasFileStream(std::shared_ptr<TimeWarpFileStream> twfstream,
    unsigned int local_object_id) {

    auto& this_objects_streams = file_streams_[local_object_id];
    for (auto& stream: this_objects_streams) {
        if (stream.get() == twfstream.get()) {
            return true;
        }
    }
    return false;
}

} // namespace warped

