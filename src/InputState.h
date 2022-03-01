#ifndef _FCITX5_MCBOPOMOFO_INPUTSTATE_H_
#define _FCITX5_MCBOPOMOFO_INPUTSTATE_H_

#include <string>
#include <vector>

namespace McBopomofo {

class InputState {
public:
    virtual ~InputState() = default;
};

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

class InputStateInputting : public InputStateNotEmpty {
public:
  void setPoppedText(std::string &poppedText);
  std::string poppedText();

protected:
  std::string m_poppedText;
};

class InputStateChoosingCandidate : public InputStateNotEmpty {
public:
  void setCandidates(std::vector<std::string> &candidates);
  std::vector<std::string> candidates();

protected:
  std::vector<std::string> m_candidates;
};

} // namespace McBopomofo

#endif
