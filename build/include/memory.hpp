#ifndef WARPED_UTILITY_MEMORY_HPP
#define WARPED_UTILITY_MEMORY_HPP

#include <memory>
#include <utility>

namespace warped {

// This is a back-port of C++14's std::make_unique<> to C++11 taken from from
// http://herbsutter.com/gotw/_102/. Once C++14 becomes standard, remove this
// function and replace any usage of it with the std version.

#ifdef __cpp_lib_make_unique
using std::make_unique;
#else
template<typename T, typename ...Args>
typename std::enable_if <!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
typename std::enable_if <std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(std::size_t n) {
    typedef typename std::remove_extent<T>::type RT;
    return std::unique_ptr<T>(new RT[n]);
}
#endif

template <class T_SRC, class T_DEST>
std::unique_ptr<T_DEST> unique_cast(std::unique_ptr<T_SRC> &&src)
{
    if (!src) return std::unique_ptr<T_DEST>();

    // Throws a std::bad_cast() if this doesn't work out
    T_DEST *dest_ptr = &dynamic_cast<T_DEST &>(*src.get());

    src.release();
    std::unique_ptr<T_DEST> ret(dest_ptr);
    return ret;
}

} // namespace warped

#endif
