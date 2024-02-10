#include "SuzhouNumbers.h"
#include "gtest/gtest.h"

namespace ChineseNumbers {
TEST(SuzhouNumberTest, Test1_1) {
  std::string output = SuzhouNumbers::Generate("0001", "0", "單位", true);
  EXPECT_EQ(output, "〡單位");
}

TEST(SuzhouNumberTest, Test1_2) {
  std::string output = SuzhouNumbers::Generate("0001", "0", "單位", false);
  EXPECT_EQ(output, "一單位");
}

TEST(SuzhouNumberTest, Test1_10) {
  std::string output = SuzhouNumbers::Generate("0010", "0", "單位", true);
  EXPECT_EQ(output, "〸單位");
}

TEST(SuzhouNumberTest, Test1234_1) {
  std::string output = SuzhouNumbers::Generate("1234", "0", "單位", true);
  EXPECT_EQ(output, "〡二〣〤\n千單位");
}

TEST(SuzhouNumberTest, Test1234_2) {
  std::string output = SuzhouNumbers::Generate("1234", "0", "單位", false);
  EXPECT_EQ(output, "一〢三〤\n千單位");
}

TEST(SuzhouNumberTest, Test12345_1) {
  std::string output = SuzhouNumbers::Generate("1234", "5", "單位", true);
  EXPECT_EQ(output, "〡二〣〤〥\n千單位");
}

TEST(SuzhouNumberTest, Test12345_2) {
  std::string output = SuzhouNumbers::Generate("1234", "5", "單位", false);
  EXPECT_EQ(output, "一〢三〤〥\n千單位");
}

}  // namespace ChineseNumbers