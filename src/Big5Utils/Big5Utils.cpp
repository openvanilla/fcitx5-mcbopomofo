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

#include "Big5Utils.h"

#include <unicode/ucnv.h>

#include <charconv>
#include <cstdint>

namespace Big5Utils {

static bool IsValidSingleUtf8Character(const char* str, int32_t length) {
  if (str == nullptr || length <= 0) {
    return false;
  }
  const uint8_t* s = reinterpret_cast<const uint8_t*>(str);
  int32_t i = 0;
  UChar32 c;
  U8_NEXT(s, i, length, c);
  if (c < 0) {  // Invalid sequence
    return false;
  }
  return i == length;  // Check if we consumed the whole string
}

std::string ConvertBig5fromUint16(uint16_t codePoint) {
  UErrorCode status = U_ZERO_ERROR;
  UConverter* conv = ucnv_open("windows-950-2000", &status);
  if (U_FAILURE(status)) {
    return "";
  }

  char big5Bytes[2];
  big5Bytes[0] = (codePoint >> 8) & 0xFF;
  big5Bytes[1] = codePoint & 0xFF;

  char utf8Buffer[16];
  int32_t utf8Length = ucnv_toAlgorithmic(
      UCNV_UTF8, conv, utf8Buffer, sizeof(utf8Buffer), big5Bytes, 2, &status);

  ucnv_close(conv);

  if (U_FAILURE(status) || utf8Length <= 0) {
    return "";
  }
  // Check if the result contains exactly one valid UTF-8 character
  if (!IsValidSingleUtf8Character(utf8Buffer, utf8Length)) {
    return "";
  }

  return std::string(utf8Buffer, utf8Length);
}

std::string ConvertBig5fromHexString(std::string hexString) {
  if (hexString.length() != 4) {
    return "";
  }

  uint16_t codePoint = 0;
  auto [ptr, ec] = std::from_chars(
      hexString.data(), hexString.data() + hexString.size(), codePoint, 16);
  if (ec != std::errc()) {
    return "";
  }

  return ConvertBig5fromUint16(codePoint);
}
}  // namespace Big5Utils