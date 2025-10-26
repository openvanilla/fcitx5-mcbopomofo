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

#include <string>

#include "RomanNumbers.h"
#include "gtest/gtest.h"

namespace RomanNumbers {

TEST(RomanNumberTest, ConvertsZeroToEmptyStringInAlphabetStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(0, RomanNumbersStyle::ALPHABETS);
  EXPECT_EQ(output, "");
}

TEST(RomanNumberTest, ConvertsZeroToEmptyStringInFullWidthUpperStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(0, RomanNumbersStyle::FULL_WIDTH_UPPER);
  EXPECT_EQ(output, "");
}

TEST(RomanNumberTest, ConvertsZeroToEmptyStringInFullWidthLowerStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(0, RomanNumbersStyle::FULL_WIDTH_LOWER);
  EXPECT_EQ(output, "");
}

TEST(RomanNumberTest, ConvertsOneToAlphabetStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(1, RomanNumbersStyle::ALPHABETS);
  EXPECT_EQ(output, "I");
}

TEST(RomanNumberTest, ConvertsOneToFullWidthUpperStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(1, RomanNumbersStyle::FULL_WIDTH_UPPER);
  EXPECT_EQ(output, "Ⅰ");
}

TEST(RomanNumberTest, ConvertsOneToFullWidthLowerStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(1, RomanNumbersStyle::FULL_WIDTH_LOWER);
  EXPECT_EQ(output, "ⅰ");
}

TEST(RomanNumberTest, Converts3999ToAlphabetStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(3999, RomanNumbersStyle::ALPHABETS);
  EXPECT_EQ(output, "MMMCMXCIX");
}

TEST(RomanNumberTest, Converts3999ToFullWidthUpperStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(3999, RomanNumbersStyle::FULL_WIDTH_UPPER);
  EXPECT_EQ(output, "ⅯⅯⅯⅭⅯⅩⅭⅨ");
}

TEST(RomanNumberTest, Converts3999ToFullWidthLowerStyle) {
  std::string output =
      RomanNumbers::ConvertFromInt(3999, RomanNumbersStyle::FULL_WIDTH_LOWER);
  EXPECT_EQ(output, "ⅿⅿⅿⅽⅿⅹⅽⅸ");
}

}  // namespace RomanNumbers
