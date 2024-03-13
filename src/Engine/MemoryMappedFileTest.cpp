// Copyright (c) 2024 and onwards The McBopomofo Authors.
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

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "MemoryMappedFile.h"
#include "gtest/gtest.h"

namespace McBopomofo {

TEST(MemoryMappedFileTest, UnopenedInstance) {
  MemoryMappedFile mf;
  EXPECT_EQ(mf.length(), 0);
  EXPECT_EQ(mf.data(), nullptr);
}

TEST(MemoryMappedFileTest, BasicFunctionalities) {
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_int_distribution<unsigned int> suffix_gen(0);

  std::string prefix("org.openvanilla.mcbopomofo.memorymappedfiletest-");
  std::filesystem::path tmp_file_path;

  constexpr int kMaxRetry = 10;
  for (int i = 0; i < kMaxRetry; ++i) {
    std::string filename = prefix + std::to_string(suffix_gen(re));
    std::filesystem::path p = std::filesystem::temp_directory_path() / filename;
    if (!std::filesystem::exists(p)) {
      tmp_file_path = p;
      break;
    }
  }
  ASSERT_FALSE(tmp_file_path.empty()) << "Must form a temp filename";

  constexpr size_t kBufSize = 4 * 1024 * 1024;
  uint8_t* buf = new uint8_t[kBufSize];

  std::uniform_int_distribution<unsigned int> randchar(0, 255);
  for (size_t i = 0; i < kBufSize; ++i) {
    buf[i] = static_cast<uint8_t>(randchar(re) & 0xff);
  }

  std::ofstream out(tmp_file_path, std::ios::binary);
  out.write(reinterpret_cast<char*>(buf), kBufSize);
  out.close();

  MemoryMappedFile mf;
  bool open_result = mf.open(tmp_file_path.c_str());
  ASSERT_TRUE(open_result);

  EXPECT_EQ(mf.length(), kBufSize);
  EXPECT_TRUE(mf.data() != nullptr);
  EXPECT_EQ(memcmp(mf.data(), buf, kBufSize), 0);

  delete[] buf;
  mf.close();
  EXPECT_EQ(mf.length(), 0);
  EXPECT_EQ(mf.data(), nullptr);

  // Should be a no-op.
  mf.close();

  std::filesystem::remove(tmp_file_path);

  // Opening a non-existence file.
  MemoryMappedFile mf2;
  open_result = mf2.open(tmp_file_path.c_str());
  EXPECT_FALSE(open_result);
  EXPECT_EQ(mf2.length(), 0);
  EXPECT_EQ(mf2.data(), nullptr);
}

}  // namespace McBopomofo
