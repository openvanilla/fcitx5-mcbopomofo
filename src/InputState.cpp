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

#include "InputState.h"

namespace McBopomofo {

std::string InputStateEmpty::composingBuffer() const { return ""; }

std::string InputStateEmptyIgnoringPrevious::composingBuffer() const {
  return "";
}

void InputStateCommitting::setPoppedText(const std::string& poppedText) {
  poppedText_ = poppedText;
}

std::string InputStateCommitting::poppedText() const { return poppedText_; }

void InputStateNotEmpty::setComposingBuffer(
    const std::string& composingBuffer) {
  composingBuffer_ = composingBuffer;
}

std::string InputStateNotEmpty::composingBuffer() const {
  return composingBuffer_;
}

void InputStateNotEmpty::setCursorIndex(size_t index) { cursorIndex_ = index; }

size_t InputStateNotEmpty::cursorIndex() const { return cursorIndex_; }

void InputStateInputting::setPoppedText(const std::string& poppedText) {
  poppedText_ = poppedText;
}

std::string InputStateInputting::poppedText() const { return poppedText_; }

void InputStateChoosingCandidate::setCandidates(
    const std::vector<std::string>& candidates) {
  candidates_ = candidates;
}

const std::vector<std::string>& InputStateChoosingCandidate::candidates()
    const {
  return candidates_;
}

}  // namespace McBopomofo
