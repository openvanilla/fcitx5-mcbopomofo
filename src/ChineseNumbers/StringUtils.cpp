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

#include "StringUtils.h"

#include <algorithm>
#include <sstream>
#include <string>

std::string StringUtils::TrimZerosAtStart(const std::string& input) {
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

std::string StringUtils::TrimZerosAtEnd(const std::string& input) {
  std::string reversed(input);
  reverse(reversed.begin(), reversed.end());
  std::string trimmed = StringUtils::TrimZerosAtStart(reversed);
  reverse(trimmed.begin(), trimmed.end());
  return trimmed;
}

std::string StringUtils::LeftPadding(const std::string& input, size_t toLength,
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
