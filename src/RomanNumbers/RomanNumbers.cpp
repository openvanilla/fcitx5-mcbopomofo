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

#include "RomanNumbers.h"

#include <array>
#include <string>
#include <string_view>

struct DigitsMap {
  std::array<std::string_view, 10> digits{};
  std::array<std::string_view, 10> tens{};
  std::array<std::string_view, 10> hundreds{};
  std::array<std::string_view, 4> thousands{};

  constexpr DigitsMap(std::array<std::string_view, 10> d,
                      std::array<std::string_view, 10> t,
                      std::array<std::string_view, 10> h,
                      std::array<std::string_view, 4> th)
      : digits(d), tens(t), hundreds(h), thousands(th) {}
};

using SV = std::string_view;

namespace {
constexpr DigitsMap kMapAlphabets{
    std::array<SV, 10>{"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII",
                       "IX"},
    std::array<SV, 10>{"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX",
                       "XC"},
    std::array<SV, 10>{"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC",
                       "CM"},
    std::array<SV, 4>{"", "M", "MM", "MMM"}};

constexpr DigitsMap kMapFullWidthUpper{
    std::array<SV, 10>{"", "Ⅰ", "Ⅱ", "Ⅲ", "Ⅳ", "Ⅴ", "Ⅵ", "Ⅶ", "Ⅷ", "Ⅸ"},
    std::array<SV, 10>{"", "Ⅹ", "ⅩⅩ", "ⅩⅩⅩ", "ⅩⅬ", "Ⅼ", "ⅬⅩ", "ⅬⅩⅩ", "ⅬⅩⅩⅩ",
                       "ⅩⅭ"},
    std::array<SV, 10>{"", "Ⅽ", "ⅭⅭ", "ⅭⅭⅭ", "ⅭⅮ", "Ⅾ", "ⅮⅭ", "ⅮⅭⅭ", "ⅮⅭⅭⅭ",
                       "ⅭⅯ"},
    std::array<SV, 4>{"", "Ⅿ", "ⅯⅯ", "ⅯⅯⅯ"}};

constexpr DigitsMap kMapFullWidthLower{
    std::array<SV, 10>{"", "ⅰ", "ⅱ", "ⅲ", "ⅳ", "ⅴ", "ⅵ", "ⅶ", "ⅷ", "ⅸ"},
    std::array<SV, 10>{"", "ⅹ", "ⅹⅹ", "ⅹⅹⅹ", "ⅹⅼ", "ⅼ", "ⅼⅹ", "ⅼⅹⅹ", "ⅼⅹⅹⅹ",
                       "ⅹⅽ"},
    std::array<SV, 10>{"", "ⅽ", "ⅽⅽ", "ⅽⅽⅽ", "ⅽⅾ", "ⅾ", "ⅾⅽ", "ⅾⅽⅽ", "ⅾⅽⅽⅽ",
                       "ⅽⅿ"},
    std::array<SV, 4>{"", "ⅿ", "ⅿⅿ", "ⅿⅿⅿ"}};
}  // namespace

static constexpr const DigitsMap& GetDigitsMap(
    RomanNumbers::RomanNumbersStyle style) {
  switch (style) {
    case RomanNumbers::RomanNumbersStyle::ALPHABETS:
      return kMapAlphabets;
    case RomanNumbers::RomanNumbersStyle::FULL_WIDTH_UPPER:
      return kMapFullWidthUpper;
    case RomanNumbers::RomanNumbersStyle::FULL_WIDTH_LOWER:
      return kMapFullWidthLower;
  }
  return kMapAlphabets;  // default
}

std::string RomanNumbers::ConvertFromInt(
    int number, RomanNumbers::RomanNumbersStyle style) {
  if (number <= 0 || number > 3999) return {};

  const auto& map = GetDigitsMap(style);

  // Unicode 有單一字元的 11/12（其餘沒有）
  if (style == RomanNumbers::RomanNumbersStyle::FULL_WIDTH_UPPER) {
    if (number == 11) return "Ⅺ";
    if (number == 12) return "Ⅻ";
  } else if (style == RomanNumbers::RomanNumbersStyle::FULL_WIDTH_LOWER) {
    if (number == 11) return "ⅺ";
    if (number == 12) return "ⅻ";
  }

  const int thou = number / 1000;
  const int hund = (number % 1000) / 100;
  const int ten = (number % 100) / 10;
  const int digit = number % 10;

  std::string result;
  result.reserve(16);
  result.append(map.thousands[thou]);
  result.append(map.hundreds[hund]);
  result.append(map.tens[ten]);
  result.append(map.digits[digit]);
  return result;
}

std::string RomanNumbers::ConvertFromString(const std::string& s,
                                            RomanNumbersStyle style) {
  if (s.empty()) return {};
  for (unsigned char ch : s) {
    if (ch < '0' || ch > '9') return {};
  }
  int value = std::atoi(s.c_str());
  if (value <= 0 || value > 3999) return {};
  return RomanNumbers::ConvertFromInt(value, style);
}
