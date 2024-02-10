#include "ChineseNumbers.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "StringUtils.h"

static std::string lowerDigits[] = {"〇", "一", "二", "三", "四",
                                    "五", "六", "七", "八", "九"};
static std::string upperDigits[] = {"零", "壹", "貳", "參", "肆",
                                    "伍", "陸", "柒", "捌", "玖"};
static std::string lowerPlaces[] = {"千", "百", "十", ""};
static std::string upperPlaces[] = {"仟", "佰", "拾", ""};
static std::string higherPlaces[] = {"",   "萬", "億", "兆", "京", "垓",
                                     "秭", "穰", "溝", "澗", "正", "載"};

static std::string convert4Digits(std::string substring,
                                  ChineseNumbers::ChineseNumberCase digitCase,
                                  bool zeroEverHappened) {
  bool zeroHappened = zeroEverHappened;
  std::stringstream output;

  for (size_t i = 0; i < substring.length(); i++) {
    char c = substring[i];
    if (c == ' ') {
      continue;
    }
    if (c == '0') {
      zeroHappened = true;
      continue;
    } else {
      if (zeroHappened) {
        if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
          output << lowerDigits[0];
        } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
          output << upperDigits[0];
        }
      }
      zeroHappened = false;
      if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
        output << lowerDigits[c - '0'];
      } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
        output << upperDigits[c - '0'];
      }
      if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
        output << lowerPlaces[i];
      } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
        output << upperPlaces[i];
      }
    }
  }
  return output.str();
}

std::string ChineseNumbers::ChineseNumbers::Generate(
    const std::string& intPart, const std::string& decPart,
    ChineseNumberCase digitCase) {
  std::string intTrimmed = StringUtils::TrimZerosAtStart(intPart);
  std::string decTrimmed = StringUtils::TrimZerosAtEnd(decPart);

  std::stringstream output;
  if (intTrimmed.empty()) {
    output << "0";
  } else {
    auto intSectionCount = (size_t)ceil((double)intTrimmed.length() / 4.0);
    size_t filledLength = intSectionCount * 4;
    std::string filled =
        StringUtils::LeftPadding(intTrimmed, filledLength, ' ');
    size_t readHead = 0;
    bool zeroEverHappen = false;
    while (readHead < filledLength) {
      std::string substring = filled.substr(readHead, 4);
      if (substring == "0000") {
        zeroEverHappen = true;
        readHead += 4;
        continue;
      }
      std::string converted =
          convert4Digits(substring, digitCase, zeroEverHappen);
      zeroEverHappen = false;
      output << converted;
      size_t place = (filledLength - readHead) / 4 - 1;
      output << higherPlaces[place];
      readHead += 4;
    }
  }

  if (!decTrimmed.empty()) {
    output << "點";
    for (char c : decTrimmed) {
      if (digitCase == ChineseNumberCase::LOWERCASE) {
        output << lowerDigits[c - '0'];
      } else if (digitCase == ChineseNumberCase::UPPERCASE) {
        output << upperDigits[c - '0'];
      }
    }
  }

  return output.str();

}  // namespace ChineseNumbers
