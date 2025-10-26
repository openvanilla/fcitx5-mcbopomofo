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

#include "ChineseNumbers.h"
#include "gtest/gtest.h"

TEST(ChineseNumberTest, GeneratesLowercaseZero) {
  std::string output = ChineseNumbers::Generate(
      "0000", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "〇");
}

TEST(ChineseNumberTest, GeneratesUppercaseZero) {
  std::string output = ChineseNumbers::Generate(
      "0000", "0", ChineseNumbers::ChineseNumberCase::UPPERCASE);
  EXPECT_EQ(output, "零");
}

TEST(ChineseNumberTest, GeneratesLowercaseOne) {
  std::string output = ChineseNumbers::Generate(
      "0001", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一");
}

TEST(ChineseNumberTest, GeneratesLowercaseEleven) {
  std::string output = ChineseNumbers::Generate(
      "0011", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一十一");
}

TEST(ChineseNumberTest, GeneratesLowercaseFourDigitNumber) {
  std::string output = ChineseNumbers::Generate(
      "1234", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一千二百三十四");
}

TEST(ChineseNumberTest, GeneratesLowercaseFiveDigitNumber) {
  std::string output = ChineseNumbers::Generate(
      "12345", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一萬二千三百四十五");
}

TEST(ChineseNumberTest, GeneratesLowercaseTenThousandOne) {
  std::string output = ChineseNumbers::Generate(
      "10001", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一萬〇一");
}
