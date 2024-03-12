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

#ifndef SRC_ENGINE_MEMORYMAPPEDFILE_H_
#define SRC_ENGINE_MEMORYMAPPEDFILE_H_

#include <cstddef>

namespace McBopomofo {

// A wrapper for managing a memory-mapped file. Access is read-only.
class MemoryMappedFile {
 public:
  ~MemoryMappedFile();
  bool open(const char* path);
  void close();

  [[nodiscard]] const char* data() const {
    return static_cast<const char*>(ptr_);
  }

  // Returns the length of the data, which is the length of the file upon open.
  [[nodiscard]] size_t length() const { return length_; }

 private:
  int fd_ = -1;          // POSIX file descriptor used by the mmap call
  void* ptr_ = nullptr;  // actual mapped data
  size_t length_ = 0;
};

}  // namespace McBopomofo

#endif  // SRC_ENGINE_MEMORYMAPPEDFILE_H_
