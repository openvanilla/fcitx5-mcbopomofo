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

#include "SuzhouNumbers.h"
#include "gtest/gtest.h"

namespace ChineseNumbers {

TEST(SuzhouNumberTest, UsesSuzhouGlyphsWhenEnabled) {
  std::string output = SuzhouNumbers::Generate("0001", "0", "單位", true);
  EXPECT_EQ(output, "〡單位");
}

TEST(SuzhouNumberTest, FallsBackToChineseDigitsWhenDisabled) {
  std::string output = SuzhouNumbers::Generate("0001", "0", "單位", false);
  EXPECT_EQ(output, "一單位");
}

TEST(SuzhouNumberTest, UsesSuzhouGlyphForTen) {
  std::string output = SuzhouNumbers::Generate("0010", "0", "單位", true);
  EXPECT_EQ(output, "〸單位");
}

TEST(SuzhouNumberTest, FormatsSuzhouDigitsAcrossRows) {
  std::string output = SuzhouNumbers::Generate("1234", "0", "單位", true);
  EXPECT_EQ(output, "〡二〣〤\n千單位");
}

TEST(SuzhouNumberTest, FormatsMixedDigitsAcrossRows) {
  std::string output = SuzhouNumbers::Generate("1234", "0", "單位", false);
  EXPECT_EQ(output, "一〢三〤\n千單位");
}

TEST(SuzhouNumberTest, AppendsSuzhouDigitsForFraction) {
  std::string output = SuzhouNumbers::Generate("1234", "5", "單位", true);
  EXPECT_EQ(output, "〡二〣〤〥\n千單位");
}

TEST(SuzhouNumberTest, AppendsChineseDigitsForFraction) {
  std::string output = SuzhouNumbers::Generate("1234", "5", "單位", false);
  EXPECT_EQ(output, "一〢三〤〥\n千單位");
}

}  // namespace ChineseNumbers
