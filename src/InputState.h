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

#ifndef SRC_INPUTSTATE_H_
#define SRC_INPUTSTATE_H_

#include <string>
#include <vector>

namespace McBopomofo {

class InputState {
 public:
  virtual ~InputState() = default;
};

class InputStateEmpty : public InputState {
 public:
  std::string composingBuffer() const;
};

class InputStateEmptyIgnoringPrevious : public InputState {
 public:
  std::string composingBuffer() const;
};

class InputStateCommitting : public InputState {
 public:
  void setPoppedText(const std::string& poppedText);
  std::string poppedText() const;

 protected:
  std::string poppedText_;
};

class InputStateNotEmpty : public InputState {
 public:
  void setComposingBuffer(const std::string& composingBuffer);
  std::string composingBuffer() const;

  void setCursorIndex(size_t cursorIndex);
  size_t cursorIndex() const;

 protected:
  std::string composingBuffer_;
  size_t cursorIndex_;
};

class InputStateInputting : public InputStateNotEmpty {
 public:
  void setPoppedText(const std::string& poppedText);
  std::string poppedText() const;

 protected:
  std::string poppedText_;
};

class InputStateChoosingCandidate : public InputStateNotEmpty {
 public:
  void setCandidates(const std::vector<std::string>& candidates);
  const std::vector<std::string>& candidates() const;

 protected:
  std::vector<std::string> candidates_;
};

}  // namespace McBopomofo

#endif  // SRC_INPUTSTATE_H_
