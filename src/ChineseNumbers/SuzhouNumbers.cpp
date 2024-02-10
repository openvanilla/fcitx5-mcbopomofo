#include "SuzhouNumbers.h"

#include <iostream>
#include <sstream>

#include "StringUtils.h"

namespace ChineseNumbers {

static std::string verticalDigits[] = {"〇", "〡", "〢", "〣", "〤",
                                       "〥", "〦", "〧", "〨", "〩"};
static std::string horizontalDigits[] = {"〇", "一", "二", "三"};
static std::string placeNames[] = {
    "",   "十",   "百",   "千",   "万", "十万", "百万", "千万",
    "億", "十億", "百億", "千億", "兆", "十兆", "百兆", "千兆",
    "京", "十京", "百京", "千京", "垓", "十垓", "百垓", "千垓",
    "秭", "十秭", "百秭", "千秭", "穰", "十穰", "百穰", "千穰",
};

std::string SuzhouNumbers::Generate(const std::string& intPart,
                                    const std::string& decPart,
                                    const std::string& unit,
                                    bool preferInitialVertical) {
  std::string intTrimmed = StringUtils::TrimZerosAtStart(intPart);
  std::string decTrimmed = StringUtils::TrimZerosAtEnd(decPart);
  std::stringstream output;
  size_t trimmedZeroCounts = 0;

  if (decTrimmed.empty()) {
    std::string trimmed = StringUtils::TrimZerosAtEnd(intTrimmed);
    trimmedZeroCounts = intTrimmed.length() - trimmed.length();
    intTrimmed = trimmed;
  }
  if (intTrimmed.empty()) {
    intTrimmed = "0";
  }

  std::string joined = intTrimmed + decTrimmed;
  bool isVertical = preferInitialVertical;
  for (char c : joined) {
    if (c == '1' || c == '2' || c == '3') {
      if (isVertical) {
        output << verticalDigits[c - '0'];
      } else {
        output << horizontalDigits[c - '0'];
      }
      isVertical = !isVertical;
    } else {
      output << verticalDigits[c - '0'];
      isVertical = preferInitialVertical;
    }
  }
  size_t joinedLength = joined.length();
  std::cout << joined << std::endl;
  std::cout << joinedLength << std::endl;
  if (joinedLength == 1 && trimmedZeroCounts == 0) {
    output << unit;
    return output.str();
  }
  if (joinedLength == 1 && trimmedZeroCounts == 1) {
    char c = intTrimmed[0];
    switch (c) {
      case '1':
        return "〸" + unit;
      case '2':
        return "〹" + unit;
      case '3':
        return "〺" + unit;
      default:
        break;
    }
  }
  size_t place = intTrimmed.length() + trimmedZeroCounts - 1;
  output << (joined.length() > 1 ? "\n" : "");
  output << placeNames[place] << unit;
  return output.str();
}
}  // namespace ChineseNumbers
