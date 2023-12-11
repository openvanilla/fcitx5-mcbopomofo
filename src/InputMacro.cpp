#include "InputMacro.h"

#include <unicode/gregocal.h>
#include <unicode/smpdtfmt.h>

#include <iostream>

namespace McBopomofo {

class InputMacroDateTodayShort : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TODAY_SHORT"; };
  std::string replacement() const;
};

class InputMacroDateTodayMedium : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TODAY_MEDIUM"; };
  std::string replacement() const;
};

class InputMacroDateTodayMediumRoc : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TODAY_MEDIUM_ROC"; };
  std::string replacement() const;
};

class InputMacroDateTodayMediumChinese : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TODAY_MEDIUM_CHINESE"; };
  std::string replacement() const;
};

class InputMacroDateYesterdayShort : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_YESTERDAY_SHORT"; };
  std::string replacement() const;
};

class InputMacroDateYesterdayMedium : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_YESTERDAY_MEDIUM"; };
  std::string replacement() const;
};

class InputMacroDateYesterdayMediumRoc : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_YESTERDAY_MEDIUM_ROC"; };
  std::string replacement() const;
};

class InputMacroDateYesterdayMediumChinese : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_YESTERDAY_MEDIUM_CHINESE"; };
  std::string replacement() const;
};

class InputMacroDateTomorrowShort : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TOMORROW_SHORT"; };
  std::string replacement() const;
};

class InputMacroDateTomorrowMedium : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TOMORROW_MEDIUM"; };
  std::string replacement() const;
};

class InputMacroDateTomorrowMediumRoc : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TOMORROW_MEDIUM_ROC"; };
  std::string replacement() const;
};

class InputMacroDateTomorrowMediumChinese : public InputMacro {
 public:
  std::string name() const { return "MACRO@DATE_TOMORROW_MEDIUM_CHINESE"; };
  std::string replacement() const;
};

class InputMacroDateTimeNowShort : public InputMacro {
 public:
  std::string name() const { return "MACRO@TIME_NOW_SHORT"; };
  std::string replacement() const;
};

class InputMacroDateTimeNowMedium : public InputMacro {
 public:
  std::string name() const { return "MACRO@TIME_NOW_MEDIUM"; };
  std::string replacement() const;
};

class InputMacroTimeZoneStandard : public InputMacro {
 public:
  std::string name() const { return "MACRO@TIMEZONE_STANDARD"; };
  std::string replacement() const;
};

class InputMacroTimeZoneShortGeneric : public InputMacro {
 public:
  std::string name() const { return "MACRO@TIMEZONE_GENERIC_SHORT"; };
  std::string replacement() const;
};

class InputMacroThisYearGanZhi : public InputMacro {
 public:
  std::string name() const { return "MACRO@THIS_YEAR_GANZHI"; };
  std::string replacement() const;
};

class InputMacroLastYearGanZhi : public InputMacro {
 public:
  std::string name() const { return "MACRO@LAST_YEAR_GANZHI"; };
  std::string replacement() const;
};

class InputMacroNextYearGanZhi : public InputMacro {
 public:
  std::string name() const { return "MACRO@NEXT_YEAR_GANZHI"; };
  std::string replacement() const;
};

class InputMacroThisYearChineseZodiac : public InputMacro {
 public:
  std::string name() const { return "MACRO@THIS_YEAR_CHINESE_ZODIAC"; };
  std::string replacement() const;
};

class InputMacroLastYearChineseZodiac : public InputMacro {
 public:
  std::string name() const { return "MACRO@LAST_YEAR_CHINESE_ZODIAC"; };
  std::string replacement() const;
};

class InputMacroNextYearChineseZodiac : public InputMacro {
 public:
  std::string name() const { return "MACRO@NEXT_YEAR_CHINESE_ZODIAC"; };
  std::string replacement() const;
};

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
  macros_.push_back(std::make_unique<InputMacroDateTimeNowShort>());
  macros_.push_back(std::make_unique<InputMacroDateTimeNowMedium>());
  macros_.push_back(std::make_unique<InputMacroTimeZoneStandard>());
  macros_.push_back(std::make_unique<InputMacroTimeZoneShortGeneric>());
  macros_.push_back(std::make_unique<InputMacroThisYearGanZhi>());
  macros_.push_back(std::make_unique<InputMacroLastYearGanZhi>());
  macros_.push_back(std::make_unique<InputMacroNextYearGanZhi>());
  macros_.push_back(std::make_unique<InputMacroThisYearChineseZodiac>());
  macros_.push_back(std::make_unique<InputMacroLastYearChineseZodiac>());
  macros_.push_back(std::make_unique<InputMacroNextYearChineseZodiac>());
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

std::string InputMacroDateTodayShort::replacement() const {
  return formatDate("", 0, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTodayMedium::replacement() const {
  return formatDate("", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumRoc::replacement() const {
  return formatDate("roc", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTodayMediumChinese::replacement() const {
  return formatDate("chinese", 0, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayShort::replacement() const {
  return formatDate("", -1, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateYesterdayMedium::replacement() const {
  return formatDate("", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayMediumRoc::replacement() const {
  return formatDate("roc", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateYesterdayMediumChinese::replacement() const {
  return formatDate("chinese", -1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowShort::replacement() const {
  return formatDate("", 1, icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTomorrowMedium::replacement() const {
  return formatDate("", 1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowMediumRoc::replacement() const {
  return formatDate("roc", 1, icu::DateFormat::EStyle::kMedium);
}

std::string InputMacroDateTomorrowMediumChinese::replacement() const {
  return formatDate("chinese", 1, icu::DateFormat::EStyle::kMedium);
}

std::string formatTime(icu::DateFormat::EStyle timeStyle) {
  UErrorCode status = U_ZERO_ERROR;
  icu::TimeZone* timezone = icu::TimeZone::createDefault();
  std::string calendarNameBase = "zh_Hant_TW";
  const icu::Locale locale =
      icu::Locale::createCanonical(calendarNameBase.c_str());
  icu::Calendar* calendar =
      icu::Calendar::createInstance(timezone, locale, status);
  calendar->setTime(icu::Calendar::getNow(), status);
  icu::DateFormat* dateFormatter = icu::DateFormat::createDateTimeInstance(
      icu::DateFormat::EStyle::kNone, timeStyle, locale);
  icu::UnicodeString formattedDate;
  icu::FieldPosition fieldPosition;
  dateFormatter->format(*calendar, formattedDate, fieldPosition);
  std::string output;
  formattedDate.toUTF8String(output);
  delete calendar;
  delete dateFormatter;
  return output;
}

std::string InputMacroDateTimeNowShort::replacement() const {
  return formatTime(icu::DateFormat::EStyle::kShort);
}

std::string InputMacroDateTimeNowMedium::replacement() const {
  return formatTime(icu::DateFormat::EStyle::kMedium);
}

std::string formatTimeZone(icu::TimeZone::EDisplayType type) {
  icu::TimeZone* timezone = icu::TimeZone::createDefault();
  const icu::Locale locale = icu::Locale::createCanonical("zh_Hant_TW");
  icu::UnicodeString formatted;
  timezone->getDisplayName(false, type, locale, formatted);
  std::string output;
  formatted.toUTF8String(output);
  delete timezone;
  return output;
}

std::string InputMacroTimeZoneStandard::replacement() const {
  return formatTimeZone(icu::TimeZone::EDisplayType::LONG_GENERIC);
}

std::string InputMacroTimeZoneShortGeneric::replacement() const {
  return formatTimeZone(icu::TimeZone::EDisplayType::SHORT_GENERIC);
}

int currentYear() {
  UErrorCode status = U_ZERO_ERROR;
  icu::GregorianCalendar calendar(icu::TimeZone::createDefault(), status);
  calendar.setTime(icu::Calendar::getNow(), status);
  int32_t year = calendar.get(UCalendarDateFields::UCAL_YEAR, status);
  return year;
}

int getYearBase(int year) {
  if (year < 4) {
    year = year * -1;
    return 60 - (year + 2) % 60;
  }
  return (year - 3) % 60;
}

std::string ganzhi(int year) {
  const std::vector<std::string> gan(
      {"癸", "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬"});
  const std::vector<std::string> zhi(
      {"亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌"});
  int base = getYearBase(year);
  int ganIndex = base % 10;
  int zhiIndex = base % 12;
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

std::string chineseZodiac(int year) {
  const std::vector<std::string> gan(
      {"水", "木", "木", "火", "火", "土", "土", "金", "金", "水"});
  const std::vector<std::string> zhi(
      {"豬", "鼠", "牛", "虎", "兔", "龍", "蛇", "馬", "羊", "猴", "雞", "狗"});
  int base = getYearBase(year);
  int ganIndex = base % 10;
  int zhiIndex = base % 12;
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

std::string InputMacroThisYearGanZhi::replacement() const {
  int year = currentYear();
  return ganzhi(year);
}

std::string InputMacroLastYearGanZhi::replacement() const {
  int year = currentYear();
  return ganzhi(year - 1);
}

std::string InputMacroNextYearGanZhi::replacement() const {
  int year = currentYear();
  return ganzhi(year + 1);
}

std::string InputMacroThisYearChineseZodiac::replacement() const {
  int year = currentYear();
  return chineseZodiac(year);
}

std::string InputMacroLastYearChineseZodiac::replacement() const {
  int year = currentYear();
  return chineseZodiac(year - 1);
}

std::string InputMacroNextYearChineseZodiac::replacement() const {
  int year = currentYear();
  return chineseZodiac(year + 1);
}

}  // namespace McBopomofo
