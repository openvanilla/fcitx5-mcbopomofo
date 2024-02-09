
#include "StringUtils.h"

#include <algorithm>
#include <sstream>

std::string ChineseNumbers::StringUtils::TrimZerosAtStart(
    const std::string& input) {
  std::stringstream output;
  bool nonZeroFound = false;
  for (char it : input) {
    if (nonZeroFound) {
      output << it;
      continue;
    }
    if (it != '0') {
      nonZeroFound = true;
      output << it;
      continue;
    }
  }
  return output.str();
}

std::string ChineseNumbers::StringUtils::TrimZerosAtEnd(
    const std::string& input) {
  std::string reversed = input;
  reverse(reversed.begin(), reversed.end());
  return StringUtils::TrimZerosAtStart(input);
}

std::string ChineseNumbers::StringUtils::LeftPadding(std::string input,
                                                     size_t toLength,
                                                     char character) {
  size_t currentLength = input.length();
  if (currentLength < toLength) {
    std::stringstream ss;
    for (size_t i = 0; i < toLength - currentLength; i++) {
      ss << character;
    }
    ss << input;
    return ss.str();
  }

  return input;
}
