#ifndef FCITX5_MCBOPOMOFO_STRINGUTILS_H
#define FCITX5_MCBOPOMOFO_STRINGUTILS_H

#include <string>

namespace StringUtils {
std::string TrimZerosAtStart(const std::string& input);
std::string TrimZerosAtEnd(const std::string& input);
std::string LeftPadding(const std::string& input, size_t toLength,
                        char character);
}  // namespace StringUtils

#endif  // FCITX5_MCBOPOMOFO_STRINGUTILS_H
