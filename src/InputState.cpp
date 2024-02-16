// Copyright (c) 2022 and onwards The McBopomofo Authors.
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

#include "InputState.h"

namespace McBopomofo {

InputStates::SelectingDateMacro::SelectingDateMacro(
    const std::function<std::string(std::string)>& converter) {
  std::string DateMacros[] = {"MACRO@DATE_TODAY_SHORT",
                              "MACRO@DATE_TODAY_MEDIUM",
                              "MACRO@DATE_TODAY_MEDIUM_ROC",
                              "MACRO@DATE_TODAY_MEDIUM_CHINESE",
                              "MACRO@DATE_TODAY_MEDIUM_JAPANESE",
                              "MACRO@THIS_YEAR_PLAIN",
                              "MACRO@THIS_YEAR_PLAIN_WITH_ERA",
                              "MACRO@THIS_YEAR_ROC",
                              "MACRO@THIS_YEAR_JAPANESE",
                              "MACRO@DATE_TODAY_WEEKDAY_SHORT",
                              "MACRO@DATE_TODAY_WEEKDAY",
                              "MACRO@DATE_TODAY2_WEEKDAY",
                              "MACRO@DATE_TODAY_WEEKDAY_JAPANESE",
                              "MACRO@TIME_NOW_SHORT",
                              "MACRO@TIME_NOW_MEDIUM",
                              "MACRO@THIS_YEAR_GANZHI",
                              "MACRO@THIS_YEAR_CHINESE_ZODIAC"};
  for (const std::string& macro : DateMacros) {
    menu.emplace_back(converter(macro));
  }
}

}  // namespace McBopomofo
