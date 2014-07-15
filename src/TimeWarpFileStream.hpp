#ifndef TIME_WARP_FILE_STREAM
#define TIME_WARP_FILE_STREAM

#include <fstream>
#include <sstream>
#include <map>
#include <mutex>
#include <atomic>

#include "TimeWarpFileStreamManager.hpp"
#include "SimulationObject.hpp"
#include "FileStream.hpp"

namespace warped {

class SimulationObject;

class TimeWarpFileStream : public FileStream {
public:
    // Constructors
    TimeWarpFileStream() = default;
    explicit TimeWarpFileStream(const std::string& filename, std::ios_base::openmode mode);
    TimeWarpFileStream (const FileStream&) = delete;
    TimeWarpFileStream (FileStream&& x);

    TimeWarpFileStream& operator= (const TimeWarpFileStream&) = delete;
    TimeWarpFileStream& operator= (TimeWarpFileStream&& rhs);

    void setCurrentTime(unsigned int current_time);

    // rollback
    void removeOutputRequestsBeforeOrAt(unsigned int rollback_time);

    // fossil collection
    void commitOutputRequestsAfterOrAt(unsigned int gvt);

    // Output operations
    virtual TimeWarpFileStream& operator<< (bool val) override;
    virtual TimeWarpFileStream& operator<< (short val) override;
    virtual TimeWarpFileStream& operator<< (unsigned short val) override;
    virtual TimeWarpFileStream& operator<< (int val) override;
    virtual TimeWarpFileStream& operator<< (unsigned int val) override;
    virtual TimeWarpFileStream& operator<< (long val) override;
    virtual TimeWarpFileStream& operator<< (unsigned long val) override;
    virtual TimeWarpFileStream& operator<< (long long val) override;
    virtual TimeWarpFileStream& operator<< (unsigned long long val) override;
    virtual TimeWarpFileStream& operator<< (float val) override;
    virtual TimeWarpFileStream& operator<< (double val) override;
    virtual TimeWarpFileStream& operator<< (long double val) override;
    virtual TimeWarpFileStream& operator<< (void* val) override;
    virtual TimeWarpFileStream& operator<< (std::streambuf* sb) override;
    virtual TimeWarpFileStream& operator<< (FileStream& (*pf)(FileStream&)) override;
    virtual TimeWarpFileStream& operator<< (std::ios& (*pf)(std::ios&)) override;
    virtual TimeWarpFileStream& operator<< (std::ios_base& (*pf)(std::ios_base&)) override;
    virtual TimeWarpFileStream& put(char c) override;
    virtual TimeWarpFileStream& write(const char* s, std::streamsize n) override;

private:

    std::multimap<unsigned int, std::string> output_requests_;

    // Needed because queues are modified by both worker threads and manager threads
    std::mutex output_requests_lock_;

    std::atomic<unsigned int> current_time_;

};

} // namespace warped

#endif
