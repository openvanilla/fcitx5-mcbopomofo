// Copyright (c) 2025 and onwards The McBopomofo Authors.
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

#ifndef SRC_PATH_COMPAT_H_
#define SRC_PATH_COMPAT_H_

#include <string>

#ifdef USE_LEGACY_FCITX5_API_STANDARDPATH
#include <fcitx-utils/standardpath.h>
#else
#include <fcitx-utils/standardpaths.h>
#endif

namespace McBopomofo {
namespace fcitx5_compat {

inline std::string locate(const std::string& path) {
#ifdef USE_LEGACY_FCITX5_API_STANDARDPATH
  return fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, path);
#else
  return fcitx::StandardPaths::global().locate(
      fcitx::StandardPathsType::PkgData, path);
#endif
}

inline std::string userDirectory() {
#ifdef USE_LEGACY_FCITX5_API_STANDARDPATH
  return fcitx::StandardPath::global().userDirectory(
      fcitx::StandardPath::Type::PkgData);
#else
  return fcitx::StandardPaths::global().userDirectory(
      fcitx::StandardPathsType::PkgData);
#endif
}

}  // namespace fcitx5_compat
}  // namespace McBopomofo

#endif  // SRC_PATH_COMPAT_H_
