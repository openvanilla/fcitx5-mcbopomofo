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

#ifndef SRC_KEY_H_
#define SRC_KEY_H_

namespace McBopomofo {

// Encapsulates the keys accepted by KeyHandler. This class never attempts to
// represent all key states that a generic input method framework desires to
// handle. Instead, this class only reflects the keys KeyHandler will handle.
//
// This is not always a perfect representation (for example, shift muddles the
// picture), but is sufficient for KeyHandler's needs.
struct Key {
  enum class KeyName { ASCII, LEFT, RIGHT, UP, DOWN, HOME, END, UNKNOWN };

  static constexpr char TAB = 9;
  static constexpr char BACKSPACE = 8;
  static constexpr char RETURN = 13;
  static constexpr char ESC = 27;
  static constexpr char SPACE = 32;
  static constexpr char DELETE = 127;

  const char ascii;
  const KeyName name;

  // Note that `ascii` takes precedence. If `ascii` is an uppercase letter or a
  // punctuation symbol produced via a shift combination, ascii is set to a
  // non-zero value and `shiftPressed` is always false. On the other hand,
  // "complex" keys such as Shift-Space will see both `ascii` and `shiftPressed`
  // set, since `ascii` alone is not sufficient to represent the key.
  const bool shiftPressed;
  const bool ctrlPressed;
  const bool isFromNumberPad;

  explicit Key(char c = 0, KeyName n = KeyName::UNKNOWN, bool isShift = false,
               bool isCtrl = false, bool isFromNumberPad = false)
      : ascii(c),
        name(n),
        shiftPressed(isShift),
        ctrlPressed(isCtrl),
        isFromNumberPad(isFromNumberPad) {}

  static Key asciiKey(char c, bool shiftPressed = false,
                      bool ctrlPressed = false, bool isFromNumberPad = false) {
    return Key(c, KeyName::ASCII, shiftPressed, ctrlPressed, isFromNumberPad);
  }

  static Key namedKey(KeyName name, bool shiftPressed = false,
                      bool ctrlPressed = false, bool isFromNumberPad = false) {
    return Key(0, name, shiftPressed, ctrlPressed, isFromNumberPad);
  }

  // Regardless of the shift state.
  [[nodiscard]] bool isCursorKeys() const {
    return name == KeyName::LEFT || name == KeyName::RIGHT ||
           name == KeyName::HOME || name == KeyName::END;
  }

  // Regardless of the shift state.
  [[nodiscard]] bool isDeleteKeys() const {
    return ascii == BACKSPACE || ascii == DELETE;
  }
};

}  // namespace McBopomofo

#endif  // SRC_KEY_H_
