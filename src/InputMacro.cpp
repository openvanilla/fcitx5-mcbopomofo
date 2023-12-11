#include "InputMacro.h"

#include <unicode/gregocal.h>
#include <unicode/smpdtfmt.h>

#include <iostream>

namespace McBopomofo {
InputMacroController::InputMacroController() {
  macros_.push_back(std::make_unique<InputMacroDateTodayShort>());
  macros_.push_back(std::make_unique<InputMacroDateTodayMedium>());
  macros_.push_back(std::make_unique<InputMacroDateTodayMediumRoc>());
  macros_.push_back(std::make_unique<InputMacroDateTodayMediumChinese>());
  macros_.push_back(std::make_unique<InputMacroDateYesterdayShort>());
  macros_.push_back(std::make_unique<InputMacroDateYesterdayMedium>());
  macros_.push_back(std::make_unique<InputMacroDateYesterdayMediumRoc>());
  macros_.push_back(std::make_unique<InputMacroDateYesterdayMediumChinese>());
  macros_.push_back(std::make_unique<InputMacroDateTomorrowShort>());
  macros_.push_back(std::make_unique<InputMacroDateTomorrowMedium>());
  macros_.push_back(std::make_unique<InputMacroDateTomorrowMediumRoc>());
  macros_.push_back(std::make_unique<InputMacroDateTomorrowMediumChinese>());
}

InputMacroController::~InputMacroController() {}

std::string InputMacroController::handle(std::string input) {
  for (auto& macro : macros_) {
    if (input == macro->name()) {
      return macro->replacement();
    }
  }
  return input;
}

std::string formatDate(std::string calendarName, int DayOffset,
                       icu::DateFormat::EStyle dateStyle) {
  UErrorCode status = U_ZERO_ERROR;
  icu::TimeZone* timezone = icu::TimeZone::createDefault();
  std::string calendarNameBase = "zh_Hant_TW";
  if (calendarName.empty() == false) {
    calendarNameBase += "@calendar=" + calendarName;
  }

  const icu::Locale locale =
      icu::Locale::createCanonical(calendarNameBase.c_str());
  icu::Calendar* calendar =
      icu::Calendar::createInstance(timezone, locale, status);
  calendar->setTime(icu::Calendar::getNow(), status);
  calendar->add(icu::Calendar::DATE, DayOffset, status);
  icu::DateFormat* dateFormatter = icu::DateFormat::createDateTimeInstance(
      dateStyle, icu::DateFormat::EStyle::kNone, locale);
  icu::UnicodeString formattedDate;
  icu::FieldPosition fieldPosition;
  dateFormatter->format(*calendar, formattedDate, fieldPosition);
  std::string output;
  formattedDate.toUTF8String(output);
  delete calendar;
  delete dateFormatter;
  return output;
}

std::string InputMacroDateTodayShort::replacement() {
  return formatDate("", 0, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTodayMedium::replacement() {
  return formatDate("", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumRoc::replacement() {
  return formatDate("roc", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumChinese::replacement() {
  return formatDate("chinese", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayShort::replacement() {
  return formatDate("", -1, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateYesterdayMedium::replacement() {
  return formatDate("", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayMediumRoc::replacement() {
  return formatDate("roc", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayMediumChinese::replacement() {
  return formatDate("chinese", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowShort::replacement() {
  return formatDate("", 1, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTomorrowMedium::replacement() {
  return formatDate("", 1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowMediumRoc::replacement() {
  return formatDate("roc", 1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowMediumChinese::replacement() {
  return formatDate("chinese", 1, icu::DateFormat::EStyle::kMedium);
}

}  // namespace McBopomofo
