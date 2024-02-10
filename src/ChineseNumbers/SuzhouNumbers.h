
#ifndef FCITX5_MCBOPOMOFO_SUZHOUNUMBERS_H
#define FCITX5_MCBOPOMOFO_SUZHOUNUMBERS_H

#include <string>

namespace ChineseNumbers::SuzhouNumbers {
std::string Generate(const std::string& intPart, const std::string& decPart, const std::string& unit,
                     bool preferInitialVertical);
}

#endif  // FCITX5_MCBOPOMOFO_SUZHOUNUMBERS_H
