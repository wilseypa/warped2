#ifndef WARPED_UTILITY_STRINGS_HPP
#define WARPED_UTILITY_STRINGS_HPP

// This header includes functions for manipulating std::strings

#include <string>

namespace warped {
    
// Create a new string with all occurrences of characters in `chars` removed
// from both ends of the original. Whitespace is trimmed by default.
std::string trim(const std::string& str, std::string chars = " \t\r\n") {
    auto start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        start = 0;
    }
    auto len = str.find_last_not_of(chars);
    if (len != std::string::npos) {
        len = len - start + 1;
    }
    auto trimmed = str.substr(start, len);
    // take care of strings that entirely composed of `chars`
    if (chars.find_first_of(trimmed.front()) != std::string::npos) {
        return "";
    }
    return trimmed;
}

// Create a new string with all occurrences of characters in `chars` removed
// from the left end of the original. Whitespace is trimmed by default.
std::string ltrim(const std::string& str, std::string chars = " \t\r\n") {
    auto start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        start = 0;
    }
    auto trimmed = str.substr(start, str.size() - start);
    if (chars.find_first_of(trimmed.front()) != std::string::npos) {
        return "";
    }
    return trimmed;
}

// Create a new string with all occurrences of characters in `chars` removed
// from the right end of the original. Whitespace is trimmed by default.
std::string rtrim(const std::string& str, std::string chars = " \t\r\n") {
    auto len = str.find_last_not_of(chars);
    if (len != std::string::npos) {
        ++len;
    }
    auto trimmed = str.substr(0, len);
    if (chars.find_first_of(trimmed.back()) != std::string::npos) {
        return "";
    }
    return trimmed;
}

} // namespace warped

#endif
