#include "TimeWarpFileStream.hpp"

namespace warped {

TimeWarpFileStream::TimeWarpFileStream(const std::string& filename, std::ios_base::openmode mode)
    : FileStream(filename, mode) {}

TimeWarpFileStream::TimeWarpFileStream (FileStream&& x) {
    *this = std::move(x);
}

TimeWarpFileStream& TimeWarpFileStream::operator= (TimeWarpFileStream&& rhs) {
    *this = std::move(rhs);
    return *this;
}

void TimeWarpFileStream::setCurrentTime(unsigned int current_time) {
    current_time_.store(current_time);
}

void TimeWarpFileStream::removeOutputRequestsAfterOrAt(unsigned int rollback_time) {
    output_requests_lock_.lock();

    if (output_requests_.empty()) {
        output_requests_lock_.unlock();
        return;
    }

    auto min = std::prev(output_requests_.end());
    while (min->first > rollback_time && min != output_requests_.begin()) {
        output_requests_.erase(min--);
    }

    if (min->first > rollback_time && min == output_requests_.begin()) {
        output_requests_.erase(min);
    }

    output_requests_lock_.unlock();
}

void TimeWarpFileStream::commitOutputRequestsBeforeOrAt(unsigned int gvt) {
    output_requests_lock_.lock();

    auto min = output_requests_.begin();

    while (min->first <= gvt && min != output_requests_.end()) {
        fstream_ << min->second;
        output_requests_.erase(min++);
    }

    output_requests_lock_.unlock();
}

std::size_t TimeWarpFileStream::size() {
    return output_requests_.size();
}

//////////////////////////// Output ////////////////////////////////////////////////////////

TimeWarpFileStream& TimeWarpFileStream::operator<< (bool val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (short val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (unsigned short val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (int val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (unsigned int val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (long val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (unsigned long val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (long long val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (unsigned long long val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (float val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (double val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (long double val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (void* val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (const char * val) {
    std::stringstream ss;
    ss << val;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (std::streambuf* sb) {
    std::stringstream ss;
    ss << sb;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (FileStream& (*pf)(FileStream&)) {
    *this << pf;
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (std::ios& (*pf)(std::ios&)) {
    std::stringstream ss;
    ss << pf;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::operator<< (std::ios_base& (*pf)(std::ios_base&)) {
    std::stringstream ss;
    ss << pf;
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::put(char c) {
    std::stringstream ss;
    ss.put(c);
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

TimeWarpFileStream& TimeWarpFileStream::write(const char* s, std::streamsize n) {
    std::stringstream ss;
    ss.write(s, n);
    output_requests_.insert(std::make_pair(current_time_.load(), ss.str()));
    return *this;
}

} // namespace warped



