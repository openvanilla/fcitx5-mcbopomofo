// Copyright (c) 2023 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "ChineseNumbers.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "StringUtils.h"

static const char* const kLowerDigits[] = {"〇", "一", "二", "三", "四",
                                           "五", "六", "七", "八", "九"};
static const char* const kUpperDigits[] = {"零", "壹", "貳", "參", "肆",
                                           "伍", "陸", "柒", "捌", "玖"};
static const char* const kLowerPlaces[] = {"千", "百", "十", ""};
static const char* const kUpperPlaces[] = {"仟", "佰", "拾", ""};
static const char* const kHigherPlaces[] = {"",   "萬", "億", "兆", "京", "垓",
                                            "秭", "穰", "溝", "澗", "正", "載"};

static std::string convert4Digits(const std::string& substring,
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
          output << kLowerDigits[0];
        } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
          output << kUpperDigits[0];
        }
      }
      zeroHappened = false;
      if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
        output << kLowerDigits[c - '0'];
      } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
        output << kUpperDigits[c - '0'];
      }
      if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
        output << kLowerPlaces[i];
      } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
        output << kUpperPlaces[i];
      }
    }
  }
  return output.str();
}

std::string ChineseNumbers::Generate(const std::string& intPart,
                                     const std::string& decPart,
                                     ChineseNumberCase digitCase) {
  std::string intTrimmed = StringUtils::TrimZerosAtStart(intPart);
  std::string decTrimmed = StringUtils::TrimZerosAtEnd(decPart);

  std::stringstream output;
  if (intTrimmed.empty()) {
    if (digitCase == ChineseNumbers::ChineseNumberCase::LOWERCASE) {
      output << kLowerDigits[0];
    } else if (digitCase == ChineseNumbers::ChineseNumberCase::UPPERCASE) {
      output << kUpperDigits[0];
    }
  } else {
    size_t intSectionCount = static_cast<size_t>(
        ceil(static_cast<double>(intTrimmed.length()) / 4.0));
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
      output << kHigherPlaces[place];
      readHead += 4;
    }
  }

  if (!decTrimmed.empty()) {
    output << "點";
    for (char c : decTrimmed) {
      if (digitCase == ChineseNumberCase::LOWERCASE) {
        output << kLowerDigits[c - '0'];
      } else if (digitCase == ChineseNumberCase::UPPERCASE) {
        output << kUpperDigits[c - '0'];
      }
    }
  }

  return output.str();
}
