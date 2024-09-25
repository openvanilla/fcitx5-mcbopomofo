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

#ifndef SRC_TIMESTAMPEDPATH_H_
#define SRC_TIMESTAMPEDPATH_H_

#include <filesystem>
#include <string>

namespace McBopomofo {

// Represents a path with a timestamp. The timestamp is not synced upon
// construction (unless it's copied or moved) and a checkTimestamp() call is
// required to sync it. This is so that first-time loading logic can be always
// built on checking timestampDifferentFromLastCheck(), which returns false for
// a newly constructed instance with an existing path. If the path does not
// exist, its timestamp is assumed to be zero.
class TimestampedPath {
 public:
  TimestampedPath() {}
  explicit TimestampedPath(const std::string& path) : path_(path) {}

  [[nodiscard]] std::filesystem::path path() const;

  bool pathExists();

  // For simplicity, this is defined as "not a directory" and does not
  // distinguish between file types (such as links). If the file does not exist,
  // this always returns false.
  bool pathIsFile();

  bool timestampDifferentFromLastCheck();
  void checkTimestamp();

 protected:
  std::filesystem::path path_;
  std::filesystem::file_time_type timestamp_ = {};
};

}  // namespace McBopomofo

#endif  // SRC_TIMESTAMPEDPATH_H_
