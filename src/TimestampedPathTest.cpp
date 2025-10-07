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

#include "TimestampedPath.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace McBopomofo {

TEST(TimestampedPathTest, DefaultConstructor) {
  TimestampedPath p;
  ASSERT_FALSE(p.pathExists());
  ASSERT_FALSE(p.pathIsFile());
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());
  p.checkTimestamp();
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());
}

TEST(TimestampedPathTest, BasicFunctionalities) {
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_int_distribution<unsigned int> suffix_gen(0);

  std::string prefix("org.openvanilla.mcbopomofo.timestamppathtest-");
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

  TimestampedPath p(tmp_file_path);
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());
  ASSERT_FALSE(p.pathExists());

  std::ofstream ofs(tmp_file_path);
  ofs << "hello, world\n";
  ofs.close();

  ASSERT_TRUE(p.pathExists());
  ASSERT_TRUE(p.pathIsFile());
  ASSERT_TRUE(p.timestampDifferentFromLastCheck());
  p.checkTimestamp();
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());

  TimestampedPath existingPath(tmp_file_path);
  ASSERT_TRUE(existingPath.pathExists());
  ASSERT_TRUE(existingPath.pathIsFile());
  ASSERT_TRUE(existingPath.timestampDifferentFromLastCheck());
  existingPath.checkTimestamp();
  ASSERT_FALSE(existingPath.timestampDifferentFromLastCheck());

  std::error_code err;
  std::filesystem::file_time_type t1 =
      std::filesystem::last_write_time(tmp_file_path, err);
  ASSERT_FALSE(err);

  ASSERT_FALSE(p.timestampDifferentFromLastCheck());

  // Advance the last write time.
  auto epoch = t1.time_since_epoch();
  std::filesystem::file_time_type t2(++epoch);
  std::filesystem::last_write_time(tmp_file_path, t2, err);
  ASSERT_FALSE(err);

  // Sanity check.
  std::filesystem::file_time_type t3 =
      std::filesystem::last_write_time(tmp_file_path, err);
  ASSERT_FALSE(err);
  ASSERT_EQ(t2, t3);
  ASSERT_GT(t3, t1);

  ASSERT_TRUE(p.timestampDifferentFromLastCheck());
  p.checkTimestamp();
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());

  TimestampedPath p2 = p;
  ASSERT_TRUE(p2.pathExists());
  ASSERT_TRUE(p2.pathIsFile());
  ASSERT_FALSE(p2.timestampDifferentFromLastCheck());

  std::filesystem::remove(tmp_file_path, err);
  ASSERT_FALSE(err);
  ASSERT_FALSE(p.pathExists());
  ASSERT_FALSE(p.pathIsFile());
  ASSERT_TRUE(p.timestampDifferentFromLastCheck());

  ASSERT_FALSE(p2.pathExists());
  ASSERT_FALSE(p2.pathIsFile());
  ASSERT_TRUE(p2.timestampDifferentFromLastCheck());

  p.checkTimestamp();
  ASSERT_FALSE(p.timestampDifferentFromLastCheck());

  p2.checkTimestamp();
  ASSERT_FALSE(p2.timestampDifferentFromLastCheck());
}

}  // namespace McBopomofo
