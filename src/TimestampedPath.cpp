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

#include "TimestampedPath.h"

namespace McBopomofo {

std::filesystem::path TimestampedPath::path() const { return path_; }

bool TimestampedPath::pathExists() {
  [[maybe_unused]] std::error_code err;
  return !path_.empty() && std::filesystem::exists(path_, err);
}

bool TimestampedPath::pathIsFile() {
  [[maybe_unused]] std::error_code err;
  return pathExists() && !std::filesystem::is_directory(path_, err);
}

bool TimestampedPath::timestampDifferentFromLastCheck() {
  if (!pathExists()) {
    std::filesystem::file_time_type zero = {};
    return timestamp_ != zero;
  }

  std::error_code err;
  std::filesystem::file_time_type t =
      std::filesystem::last_write_time(path_, err);
  if (err) {
    return true;
  }

  return t != timestamp_;
}

void TimestampedPath::checkTimestamp() {
  if (pathExists()) {
    std::error_code err;
    std::filesystem::file_time_type t =
        std::filesystem::last_write_time(path_, err);
    if (err) {
      timestamp_ = {};
    } else {
      timestamp_ = t;
    }
  } else {
    timestamp_ = {};
  }
}

}  // namespace McBopomofo
