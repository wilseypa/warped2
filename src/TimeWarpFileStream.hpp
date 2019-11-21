#ifndef TIME_WARP_FILE_STREAM
#define TIME_WARP_FILE_STREAM

#include <fstream>
#include <sstream>
#include <map>
#include <mutex>
#include <atomic>

#include "TimeWarpFileStreamManager.hpp"
#include "LogicalProcess.hpp"
#include "FileStream.hpp"

/* This is a sub class of FileStream which will override all of the output methods. In the time
 * warp mechanism, output cannot be commited until GVT passes the lvt of each lp. Instead of
 * directly committing an output, a queue will be kept of output requests with a timestamp for
 * each request. When fossil collection occurs the output requests will be committed using the
 * fstream lp in the FileStream base class. The LogicalProcess will produce the output
 * request while the TimeWarpFileStreamManager will perform rollbacks, Fossil collection, and
 * ensure that the current times are updated.
 */

namespace warped {

class LogicalProcess;

class TimeWarpFileStream : public FileStream {
public:
    // Constructors
    TimeWarpFileStream() = default;
    explicit TimeWarpFileStream(const std::string& filename, std::ios_base::openmode mode);
    TimeWarpFileStream (const FileStream&) = delete;
    TimeWarpFileStream (FileStream&& x);

    TimeWarpFileStream& operator= (const TimeWarpFileStream&) = delete;
    TimeWarpFileStream& operator= (TimeWarpFileStream&& rhs);

    void setCurrentEvent(std::shared_ptr<Event> current_event);

    // rollback
    void removeOutputRequestsAfter(std::shared_ptr<Event> rollback_event);

    // fossil collection
    void commitOutputRequestsBefore(unsigned int gvt);

    std::size_t size();

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
    virtual TimeWarpFileStream& operator<< (const char * val) override;
    virtual TimeWarpFileStream& operator<< (std::streambuf* sb) override;
    // virtual TimeWarpFileStream& operator<< (FileStream& (*pf)(FileStream&)) override;
    virtual TimeWarpFileStream& operator<< (std::ios& (*pf)(std::ios&)) override;
    virtual TimeWarpFileStream& operator<< (std::ios_base& (*pf)(std::ios_base&)) override;
    virtual TimeWarpFileStream& put(char c) override;
    virtual TimeWarpFileStream& write(const char* s, std::streamsize n) override;

private:

    std::vector<std::pair<std::shared_ptr<Event>, std::string>> output_requests_;

    // Needed because queues are modified by both worker threads and manager threads
    std::mutex output_requests_lock_;

    std::shared_ptr<Event> current_event_;

};

} // namespace warped

#endif
