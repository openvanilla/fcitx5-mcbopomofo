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

struct InputState {
  virtual ~InputState() = default;
};

namespace InputStates {

// Empty state, the ground state of a state machine.
//
// When a state machine implementation enters this state, it may produce a side
// effect with the previous state. For example, if the previous state is
// Inputting, and an implementation enters Empty, the implementation may commit
// whatever is in Inputting to the input method context.
struct Empty : InputState {};

// Empty state with no consideration for any previous state.
//
// When a state machine implementation enters this state, it must not produce
// any side effect. In other words, any previous state is discarded. An
// implementation must continue to enter Empty after this, so that no use sites
// of the state machine need to check for both Empty and EmptyIgnoringPrevious
// states.
struct EmptyIgnoringPrevious : InputState {};

// Committing text.
struct Committing : InputState {
  explicit Committing(const std::string& t) : text(t) {}

  const std::string text;
};

// NotEmpty state that has a non-empty composing buffer ("preedit" in some IME
// frameworks).
struct NotEmpty : InputState {
  NotEmpty(const std::string& buf, const size_t index)
      : composingBuffer(buf), cursorIndex(index) {}

  const std::string composingBuffer;
  const size_t cursorIndex;
};

// Inputting state with an optional field to commit evicted ("popped") segments
// in the composing buffer.
struct Inputting : NotEmpty {
  Inputting(const std::string& buf, const size_t index)
      : NotEmpty(buf, index) {}

  std::string evictedText;
};

// Candidate selecting state with a non-empty composing buffer.
struct ChoosingCandidate : NotEmpty {
  ChoosingCandidate(const std::string& buf, const size_t index,
                    const std::vector<std::string>& cs)
      : NotEmpty(buf, index), candidates(cs) {}

  const std::vector<std::string> candidates;
};

}  // namespace InputStates

}  // namespace McBopomofo

#endif  // SRC_INPUTSTATE_H_
