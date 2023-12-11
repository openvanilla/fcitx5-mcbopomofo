#ifndef INPUTMACRO
#define INPUTMACRO

#include <memory>
#include <string>
#include <vector>

namespace McBopomofo {
class InputMacro {
 public:
  virtual std::string name() = 0;
  virtual std::string replacement() = 0;
};

class InputMacroController {
 public:
  InputMacroController();
  ~InputMacroController();
  std::string handle(std::string input);

 private:
  std::vector<std::unique_ptr<InputMacro>> macros_;
};

class InputMacroDateTodayShort : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TODAY_SHORT"); };
  std::string replacement();
};

class InputMacroDateTodayMedium : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TODAY_MEDIUM"); };
  std::string replacement();
};

class InputMacroDateTodayMediumRoc : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TODAY_MEDIUM_ROC"); };
  std::string replacement();
};

class InputMacroDateTodayMediumChinese : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TODAY_MEDIUM_CHINESE"); };
  std::string replacement();
};

class InputMacroDateYesterdayShort : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_YESTERDAY_SHORT"); };
  std::string replacement();
};

class InputMacroDateYesterdayMedium : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_YESTERDAY_MEDIUM"); };
  std::string replacement();
};

class InputMacroDateYesterdayMediumRoc : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_YESTERDAY_MEDIUM_ROC"); };
  std::string replacement();
};

class InputMacroDateYesterdayMediumChinese : public InputMacro {
 public:
  std::string name() {
    return std::string("MACRO@DATE_YESTERDAY_MEDIUM_CHINESE");
  };
  std::string replacement();
};

class InputMacroDateTomorrowShort : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TOMORROW_SHORT"); };
  std::string replacement();
};

class InputMacroDateTomorrowMedium : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TOMORROW_MEDIUM"); };
  std::string replacement();
};

class InputMacroDateTomorrowMediumRoc : public InputMacro {
 public:
  std::string name() { return std::string("MACRO@DATE_TOMORROW_MEDIUM_ROC"); };
  std::string replacement();
};

class InputMacroDateTomorrowMediumChinese : public InputMacro {
 public:
  std::string name() {
    return std::string("MACRO@DATE_TOMORROW_MEDIUM_CHINESE");
  };
  std::string replacement();
};

}  // namespace McBopomofo

#endif /* INPUTMACRO */
