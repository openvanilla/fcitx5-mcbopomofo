#include "ChineseNumbers.h"
#include "gtest/gtest.h"

TEST(ChineseNumberTest, Test1) {
  std::string output =
      ChineseNumbers::Generate("0001", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一");
}

TEST(ChineseNumberTest, Test11) {
  std::string output =
      ChineseNumbers::Generate("0011", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一十一");
}

TEST(ChineseNumberTest, Test1234) {
  std::string output =
      ChineseNumbers::Generate("1234", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一千二百三十四");
}

TEST(ChineseNumberTest, Test12345) {
  std::string output =
      ChineseNumbers::Generate("12345", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一萬二千三百四十五");
}

TEST(ChineseNumberTest, Test10001) {
  std::string output =
      ChineseNumbers::Generate("10001", "0", ChineseNumbers::ChineseNumberCase::LOWERCASE);
  EXPECT_EQ(output, "一萬〇一");
}
