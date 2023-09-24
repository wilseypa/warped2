#ifndef WARPED_UTILITY_WARNINGS_HPP
#define WARPED_UTILITY_WARNINGS_HPP

namespace warped {

// This template will silence unused parameter warnings.
template<class... T> void unused(T&& ...) {}

} // namespace warped

#endif
