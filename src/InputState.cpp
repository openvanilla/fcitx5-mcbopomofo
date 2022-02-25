#include "InputState.h"

namespace McBopomofo {

std::string InputStateEmpty::composingBuffer() { return ""; }

std::string InputStateEmptyIgnoringPrevious::composingBuffer() { return ""; }

void InputStateCommitting::setPoppedText(std::string &poppedText) {
  m_poppedText = poppedText;
}

std::string InputStateCommitting::poppedText() { return m_poppedText; }

void InputStateNotEmpty::setComposingBuffer(std::string &composingBuffer) {
  m_composingBuffer = composingBuffer;
}

std::string InputStateNotEmpty::composingBuffer() { return m_composingBuffer; }

void InputStateNotEmpty::setCursorIndex(size_t index) { m_cursorIndex = index; }

size_t InputStateNotEmpty::cursorIndex() { return m_cursorIndex; }

} // namespace McBopomofo