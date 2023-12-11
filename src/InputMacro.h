#ifndef INPUTMACRO
#define INPUTMACRO

#include <memory>
#include <string>
#include <unordered_map>

namespace McBopomofo {
class InputMacro {
 public:
  virtual std::string name() const = 0;
  virtual std::string replacement() const = 0;
};

class InputMacroController {
 public:
  InputMacroController();
  ~InputMacroController();
  std::string handle(std::string input);

 private:
  std::unordered_map<std::string, std::unique_ptr<InputMacro>> macros_;
};

}  // namespace McBopomofo

#endif /* INPUTMACRO */
