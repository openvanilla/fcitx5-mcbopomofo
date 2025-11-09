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
#include <gtest/gtest.h>

#include "Big5Utils.h"

namespace Big5Utils {

TEST(Big5UtilsTest, ConvertBig5fromUint16_ValidCodePoint) {
  // Test with a valid Big5 code point for "中" (0xA4A4)
  std::string result = convertBig5fromUint16(0xA4A4);
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result, "中");
}

TEST(Big5UtilsTest, ConvertBig5fromUint16_AnotherValidCodePoint) {
  // Test with a valid Big5 code point for "文" (0xA4E5)
  std::string result = convertBig5fromUint16(0xA4E5);
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result, "文");
}

TEST(Big5UtilsTest, ConvertBig5fromUint16_InvalidCodePoint) {
  // Test with an invalid Big5 code point
  std::string result = convertBig5fromUint16(0x0000);
  EXPECT_TRUE(result.empty());
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_ValidHexString) {
  // Test with valid hex string "A4A4" for "中"
  std::string result = convertBig5fromHexString("A4A4");
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result, "中");
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_AnotherValidHexString) {
  // Test with valid hex string "A4E5" for "文"
  std::string result = convertBig5fromHexString("A4E5");
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result, "文");
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_LowercaseHex) {
  // Test with lowercase hex string
  std::string result = convertBig5fromHexString("a4a4");
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result, "中");
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_TooShort) {
  // Test with hex string that is too short
  std::string result = convertBig5fromHexString("A4A");
  EXPECT_TRUE(result.empty());
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_Empty) {
  // Test with empty string
  std::string result = convertBig5fromHexString("");
  EXPECT_TRUE(result.empty());
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_LongerThanNeeded) {
  // Test with hex string longer than 4 characters (should still work)
  std::string result = convertBig5fromHexString("A4A400");
  EXPECT_FALSE(result.empty());
}

TEST(Big5UtilsTest, ConvertBig5fromHexString_InvalidHexString) {
  // Test with invalid Big5 hex string
  std::string result = convertBig5fromHexString("0000");
  EXPECT_TRUE(result.empty());
}

}  // namespace Big5Utils
