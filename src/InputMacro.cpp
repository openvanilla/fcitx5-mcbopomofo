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

std::string formatDate(icu::Calendar* calendar, int DayOffset,
                       icu::DateFormat::EStyle dateStyle) {
  UErrorCode status = U_ZERO_ERROR;
  calendar->setTime(icu::Calendar::getNow(), status);
  calendar->add(icu::Calendar::DATE, DayOffset, status);
  icu::Locale locale = icu::Locale::createCanonical("zh_Hant_TW");
  icu::DateFormat* dateFormatter = icu::DateFormat::createDateTimeInstance(
      dateStyle, icu::DateFormat::EStyle::kNone, locale);
  icu::UnicodeString formattedDate;
  icu::FieldPosition fieldPosition;
  dateFormatter->format(*calendar, formattedDate, fieldPosition);
  std::string output;
  formattedDate.toUTF8String(output);
  delete dateFormatter;
  return output;
}

std::string InputMacroDateTodayShort::replacement() {
  UErrorCode status = U_ZERO_ERROR;
  icu::GregorianCalendar calendar(status);
  return formatDate(&calendar, 0, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTodayMedium::replacement() {
  UErrorCode status = U_ZERO_ERROR;
  icu::GregorianCalendar calendar(status);
  return formatDate(&calendar, 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumRoc::replacement() {
  UErrorCode status = U_ZERO_ERROR;
  const icu::Locale locale =
      icu::Locale::createCanonical("zh_Hant_TW@calendar=republic_of_china");
  icu::Calendar* calendar = icu::Calendar::createInstance(locale, status);
  if (status) {
    std::cout << "Failed to create chinese calendar" << status << std::endl;
  }
  return formatDate(calendar, 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumChinese::replacement() {
  UErrorCode status = U_ZERO_ERROR;
  const icu::Locale locale =
      icu::Locale::createCanonical("zh_Hant_TW@calendar=chinese");
  icu::Calendar* calendar = icu::Calendar::createInstance(locale, status);

  if (status) {
    std::cout << "Failed to create chinese calendar" << status << std::endl;
  }
  return formatDate(calendar, 0, icu::DateFormat::EStyle::kMedium);
}

}  // namespace McBopomofo
