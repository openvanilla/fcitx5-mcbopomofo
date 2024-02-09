#ifndef FCITX5_MCBOPOMOFO_CHINESENUMBER_H
#define FCITX5_MCBOPOMOFO_CHINESENUMBER_H

#include <string>
namespace ChineseNumbers {
enum class ChineseNumberCase { LOWERCASE, UPPERCASE };

namespace ChineseNumbers {
std::string Generate(const std::string& intPart, const std::string& decPart,
                     ChineseNumberCase digitCase);
};
}  // namespace ChineseNumbers

#endif  // FCITX5_MCBOPOMOFO_CHINESENUMBER_H
