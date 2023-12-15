#include "InputMacro.h"

#include <unicode/gregocal.h>
#include <unicode/smpdtfmt.h>

#include <functional>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace McBopomofo {

std::string formatDate(std::string calendarName, int DayOffset,
                       icu::DateFormat::EStyle dateStyle);
std::string formatTime(icu::DateFormat::EStyle timeStyle);
std::string formatTimeZone(icu::TimeZone::EDisplayType type);
int currentYear();
std::string ganzhi(int year);
std::string chineseZodiac(int year);

class InputMacroDate : public InputMacro {
 public:
  InputMacroDate(std::string macroName, std::string calendar, int offset,
                 icu::DateFormat::EStyle style)
      : name_(macroName),
        calendarName_(calendar),
        DayOffset_(offset),
        dateStyle_(style) {}
  std::string name() const override { return name_; }
  std::string replacement() const override {
    return formatDate(calendarName_, DayOffset_, dateStyle_);
  }

 private:
  std::string name_;
  std::string calendarName_;
  int DayOffset_;
  icu::DateFormat::EStyle dateStyle_;
};

class InputMacroDateTime : public InputMacro {
 public:
  InputMacroDateTime(std::string macroName, icu::DateFormat::EStyle style)
      : name_(macroName), timeStyle_(style) {}
  std::string name() const override { return name_; }
  std::string replacement() const override {
    return formatTime(timeStyle_);
  }

 private:
  std::string name_;
  icu::DateFormat::EStyle timeStyle_;
};

class InputMacroTimeZone : public InputMacro {
 public:
  InputMacroTimeZone(std::string macroName, icu::TimeZone::EDisplayType type)
      : name_(macroName), type_(type) {}
  std::string name() const override { return name_; }
  std::string replacement() const override {
    return formatTimeZone(type_);
  }

 private:
  std::string name_;
  icu::TimeZone::EDisplayType type_;
};


// transform is a function that takes an int and returns a string
template <typename transform>
class InputMacroTransform : public InputMacro {
 public:
  InputMacroTransform(std::string macroName, int yearOffset, transform t)
      : name_(macroName), yearOffset_(yearOffset), t_(t) {}
  std::string name() const override { return name_; }
  std::string replacement() const override {
    int year = currentYear();
    return t_(year + yearOffset_);
  }

 private:
  std::string name_;
  int yearOffset_;
  transform t_;
};

// currying
class InputMacroGanZhi : public InputMacroTransform<std::function<decltype(ganzhi)>> {
  public:
  InputMacroGanZhi(std::string macroName, int yearOffset)
      : InputMacroTransform(macroName, yearOffset, ganzhi) {}
};

class InputMacroZodiac : public InputMacroTransform<std::function<decltype(chineseZodiac)>> {
  public:
  InputMacroZodiac(std::string macroName, int yearOffset)
      : InputMacroTransform(macroName, yearOffset, chineseZodiac) {}
};

class InputMacroDateTodayShort : public InputMacroDate {
  public:
  InputMacroDateTodayShort()
      : InputMacroDate("MACRO@DATE_TODAY_SHORT", "", 0,
                       icu::DateFormat::EStyle::kShort) {}
};

class InputMacroDateTodayMedium : public InputMacroDate {
  public:
  InputMacroDateTodayMedium()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM", "", 0,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTodayMediumRoc : public InputMacroDate {
  public:
  InputMacroDateTodayMediumRoc()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_ROC", "roc", 0,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTodayMediumChinese : public InputMacroDate {
  public:
  InputMacroDateTodayMediumChinese()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_CHINESE", "chinese", 0,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTodayMediumJapanese : public InputMacroDate {
  public:
  InputMacroDateTodayMediumJapanese()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_JAPANESE", "japanese", 0,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateYesterdayShort : public InputMacroDate {
  public:
  InputMacroDateYesterdayShort()
      : InputMacroDate("MACRO@DATE_YESTERDAY_SHORT", "", -1,
                       icu::DateFormat::EStyle::kShort) {}
};

class InputMacroDateYesterdayMedium : public InputMacroDate {
  public:
  InputMacroDateYesterdayMedium()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM", "", -1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateYesterdayMediumRoc : public InputMacroDate {
  public:
  InputMacroDateYesterdayMediumRoc()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_ROC", "roc", -1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateYesterdayMediumChinese : public InputMacroDate {
  public:
  InputMacroDateYesterdayMediumChinese()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_CHINESE", "chinese", -1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateYesterdayMediumJapanese : public InputMacroDate {
  public:
  InputMacroDateYesterdayMediumJapanese()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_JAPANESE", "japanese", -1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTomorrowShort : public InputMacroDate {
  public:
  InputMacroDateTomorrowShort()
      : InputMacroDate("MACRO@DATE_TOMORROW_SHORT", "", 1,
                       icu::DateFormat::EStyle::kShort) {}
};

class InputMacroDateTomorrowMedium : public InputMacroDate {
  public:
  InputMacroDateTomorrowMedium()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM", "", 1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTomorrowMediumRoc : public InputMacroDate {
  public:
  InputMacroDateTomorrowMediumRoc()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_ROC", "roc", 1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTomorrowMediumChinese : public InputMacroDate {
  public:
  InputMacroDateTomorrowMediumChinese()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_CHINESE", "chinese", 1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTomorrowMediumJapanese : public InputMacroDate {
  public:
  InputMacroDateTomorrowMediumJapanese()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_JAPANESE", "japanese", 1,
                       icu::DateFormat::EStyle::kMedium) {}
};

class InputMacroDateTimeNowShort : public InputMacroDateTime {
  public:
  InputMacroDateTimeNowShort()
      : InputMacroDateTime("MACRO@TIME_NOW_SHORT",
                           icu::DateFormat::EStyle::kShort) {}
};

class InputMacroDateTimeNowMedium : public InputMacroDateTime {
  public:
  InputMacroDateTimeNowMedium()
      : InputMacroDateTime("MACRO@TIME_NOW_MEDIUM",
                           icu::DateFormat::EStyle::kMedium) {}
};


class InputMacroTimeZoneStandard : public InputMacroTimeZone {
  public:
  InputMacroTimeZoneStandard()
      : InputMacroTimeZone("MACRO@TIMEZONE_STANDARD",
                           icu::TimeZone::EDisplayType::LONG_GENERIC) {}
};


class InputMacroTimeZoneShortGeneric : public InputMacroTimeZone {
  public:
  InputMacroTimeZoneShortGeneric()
      : InputMacroTimeZone("MACRO@TIMEZONE_GENERIC_SHORT",
                           icu::TimeZone::EDisplayType::SHORT_GENERIC) {}
};

class InputMacroThisYearGanZhi : public InputMacroGanZhi {
  public:
  InputMacroThisYearGanZhi()
      : InputMacroGanZhi("MACRO@THIS_YEAR_GANZHI", 0) {}
};

class InputMacroLastYearGanZhi : public InputMacroGanZhi {
  public:
  InputMacroLastYearGanZhi()
      : InputMacroGanZhi("MACRO@LAST_YEAR_GANZHI", -1) {}
};

class InputMacroNextYearGanZhi : public InputMacroGanZhi {
  public:
  InputMacroNextYearGanZhi()
      : InputMacroGanZhi("MACRO@NEXT_YEAR_GANZHI", 1) {}
};

class InputMacroThisYearChineseZodiac : public InputMacroZodiac {
  public:
  InputMacroThisYearChineseZodiac()
      : InputMacroZodiac("MACRO@THIS_YEAR_CHINESE_ZODIAC", 0) {}
};

class InputMacroLastYearChineseZodiac : public InputMacroZodiac {
  public:
  InputMacroLastYearChineseZodiac()
      : InputMacroZodiac("MACRO@LAST_YEAR_CHINESE_ZODIAC", -1) {}
};

class InputMacroNextYearChineseZodiac : public InputMacroZodiac {
  public:
  InputMacroNextYearChineseZodiac()
      : InputMacroZodiac("MACRO@NEXT_YEAR_CHINESE_ZODIAC", 1) {}
};

static void AddMacro(
    std::unordered_map<std::string, std::unique_ptr<InputMacro>>& m,
    std::unique_ptr<InputMacro> p) {
  m.insert({p->name(), std::move(p)});
}

InputMacroController::InputMacroController() {
  AddMacro(macros_, std::make_unique<InputMacroDateTodayShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTimeNowShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTimeNowMedium>());
  AddMacro(macros_, std::make_unique<InputMacroTimeZoneStandard>());
  AddMacro(macros_, std::make_unique<InputMacroTimeZoneShortGeneric>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearChineseZodiac>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearChineseZodiac>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearChineseZodiac>());
}

InputMacroController::~InputMacroController() {}

std::string InputMacroController::handle(std::string input) {
  const auto& it = macros_.find(input);
  if (it != macros_.cend()) {
    return it->second->replacement();
  }
  return input;
}

std::string formatDate(std::string calendarName, int DayOffset,
                       icu::DateFormat::EStyle dateStyle) {
  UErrorCode status = U_ZERO_ERROR;
  icu::TimeZone* timezone = icu::TimeZone::createDefault();
  std::string calendarNameBase =
      calendarName == "japanese" ? "ja_JP" : "zh_Hant_TW";
  if (!calendarName.empty()) {
    calendarNameBase += "@calendar=" + calendarName;
  }

  const icu::Locale locale =
      icu::Locale::createCanonical(calendarNameBase.c_str());
  std::unique_ptr<icu::Calendar> calendar(icu::Calendar::createInstance(timezone, locale, status));
  calendar->setTime(icu::Calendar::getNow(), status);
  calendar->add(icu::Calendar::DATE, DayOffset, status);
  std::unique_ptr<icu::DateFormat> dateFormatter(icu::DateFormat::createDateTimeInstance(
      dateStyle, icu::DateFormat::EStyle::kNone, locale));
  icu::UnicodeString formattedDate;
  icu::FieldPosition fieldPosition;
  dateFormatter->format(*calendar, formattedDate, fieldPosition);
  std::string output;
  formattedDate.toUTF8String(output);
  return output;
}

std::string formatTime(icu::DateFormat::EStyle timeStyle) {
  UErrorCode status = U_ZERO_ERROR;
  icu::TimeZone* timezone = icu::TimeZone::createDefault();
  std::string calendarNameBase = "zh_Hant_TW";
  const icu::Locale locale =
      icu::Locale::createCanonical(calendarNameBase.c_str());
  std::unique_ptr<icu::Calendar> calendar(icu::Calendar::createInstance(timezone, locale, status));
  calendar->setTime(icu::Calendar::getNow(), status);
  std::unique_ptr<icu::DateFormat> dateFormatter(icu::DateFormat::createDateTimeInstance(
      icu::DateFormat::EStyle::kNone, timeStyle, locale));
  icu::UnicodeString formattedDate;
  icu::FieldPosition fieldPosition;
  dateFormatter->format(*calendar, formattedDate, fieldPosition);
  std::string output;
  formattedDate.toUTF8String(output);
  return output;
}

std::string formatTimeZone(icu::TimeZone::EDisplayType type) {
  std::unique_ptr<icu::TimeZone> timezone(icu::TimeZone::createDefault());
  const icu::Locale locale = icu::Locale::createCanonical("zh_Hant_TW");
  icu::UnicodeString formatted;
  timezone->getDisplayName(false, type, locale, formatted);
  std::string output;
  formatted.toUTF8String(output);
  return output;
}

int currentYear() {
  UErrorCode status = U_ZERO_ERROR;
  icu::GregorianCalendar calendar(icu::TimeZone::createDefault(), status);
  calendar.setTime(icu::Calendar::getNow(), status);
  int32_t year = calendar.get(UCalendarDateFields::UCAL_YEAR, status);
  return year;
}

// NOLINTBEGIN(readability-magic-numbers)
int getYearBase(int year) {
  if (year < 4) {
    year = year * -1;
    return 60 - (year + 2) % 60;
  }
  return (year - 3) % 60;
}
// NOLINTEND(readability-magic-numbers)

std::string ganzhi(int year) {
  const std::vector<std::string> gan(
      {"癸", "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬"});
  const std::vector<std::string> zhi(
      {"亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌"});
  size_t base = static_cast<size_t>(getYearBase(year));
  size_t ganIndex = base % gan.size();
  size_t zhiIndex = base % zhi.size();
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

std::string chineseZodiac(int year) {
  const std::vector<std::string> gan(
      {"水", "木", "木", "火", "火", "土", "土", "金", "金", "水"});
  const std::vector<std::string> zhi(
      {"豬", "鼠", "牛", "虎", "兔", "龍", "蛇", "馬", "羊", "猴", "雞", "狗"});
  size_t base = static_cast<size_t>(getYearBase(year));
  size_t ganIndex = base % gan.size();
  size_t zhiIndex = base % zhi.size();
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

}  // namespace McBopomofo
