#ifndef FCITX5_MCBOPOMOFO_STRINGUTILS_H
#define FCITX5_MCBOPOMOFO_STRINGUTILS_H

#include <string>

namespace ChineseNumbers::StringUtils {
std::string TrimZerosAtStart(const std::string& input);
std::string TrimZerosAtEnd(const std::string& input);
std::string LeftPadding(std::string input, size_t toLength, char character);
}  // namespace ChineseNumbers::StringUtils

#endif  // FCITX5_MCBOPOMOFO_STRINGUTILS_H
