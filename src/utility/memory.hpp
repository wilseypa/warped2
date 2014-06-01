#ifndef WARPED_UTILITY_MEMORY_HPP
#define WARPED_UTILITY_MEMORY_HPP

#include <memory>
#include <utility>

namespace warped {

// This is a back-port of C++14's std::make_unique<> to C++11 taken from from
// http://herbsutter.com/gotw/_102/. Once C++14 becomes standard, remove this
// function and replace any usage of it with the std version.
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace warped

#endif
