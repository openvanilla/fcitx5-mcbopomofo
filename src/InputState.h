#ifndef _FCITX5_MCBOPOMOFO_INPUTSTATE_H_
#define _FCITX5_MCBOPOMOFO_INPUTSTATE_H_

#include <string>

namespace McBopomofo {

class InputState {};

class InputStateEmpty : public InputState {
public:
  std::string composingBuffer();
};

class InputStateEmptyIgnoringPrevious : public InputState {
public:
  std::string composingBuffer();
};

class InputStateCommitting : public InputState {
public:
  void setPoppedText(std::string &poppedText);
  std::string poppedText();

protected:
  std::string m_poppedText;
};

class InputStateNotEmpty : public InputState {
public:
  void setComposingBuffer(std::string &composingBuffer);
  std::string composingBuffer();

  void setCursorIndex(size_t cursorIndex);
  size_t cursorIndex();

protected:
  std::string m_composingBuffer;
  size_t m_cursorIndex;
};

} // namespace McBopomofo

#endif
