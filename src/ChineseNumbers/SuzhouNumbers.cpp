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

#include "SuzhouNumbers.h"

#include <iostream>
#include <sstream>

#include "StringUtils.h"

static const char* const kVerticalDigits[] = {"〇", "〡", "〢", "〣", "〤",
                                              "〥", "〦", "〧", "〨", "〩"};
static const char* const kHorizontalDigits[] = {"〇", "一", "二", "三"};
static const char* const kPlaceNames[] = {
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
        output << kVerticalDigits[c - '0'];
      } else {
        output << kHorizontalDigits[c - '0'];
      }
      isVertical = !isVertical;
    } else {
      output << kVerticalDigits[c - '0'];
      isVertical = preferInitialVertical;
    }
  }
  size_t joinedLength = joined.length();
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
  output << kPlaceNames[place] << unit;
  return output.str();
}
