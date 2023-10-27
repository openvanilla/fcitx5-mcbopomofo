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
#include <string_view>
#include <utility>
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
  explicit Committing(std::string t) : text(std::move(t)) {}

  const std::string text;
};

// NotEmpty state that has a non-empty composing buffer ("preedit" in some IME
// frameworks).
struct NotEmpty : InputState {
  NotEmpty(std::string buf, const size_t index,
           const std::string_view& tooltipText = "")
      : composingBuffer(std::move(buf)),
        cursorIndex(index),
        tooltip(tooltipText) {}

  const std::string composingBuffer;

  // UTF-8 based cursor index.
  const size_t cursorIndex;

  const std::string tooltip;
};

// Inputting state.
struct Inputting : NotEmpty {
  Inputting(const std::string& buf, const size_t index,
            const std::string_view& tooltipText = "")
      : NotEmpty(buf, index, tooltipText) {}
};

// Candidate selecting state with a non-empty composing buffer.
struct ChoosingCandidate : NotEmpty {
  struct Candidate;

  ChoosingCandidate(const std::string& buf, const size_t index,
                    std::vector<Candidate> cs)
      : NotEmpty(buf, index), candidates(std::move(cs)) {}

  const std::vector<Candidate> candidates;

  struct Candidate {
    Candidate(std::string r, std::string v)
        : reading(std::move(r)), value(std::move(v)) {}
    const std::string reading;
    const std::string value;
  };
};

inline bool operator==(const ChoosingCandidate::Candidate& a,
                       const ChoosingCandidate::Candidate& b) {
  return a.reading == b.reading && a.value == b.value;
}

// Represents the Marking state where the user uses Shift-Left/Shift-Right to
// mark a phrase to be added to their custom phrases. A Marking state still has
// a composingBuffer, and the invariant is that composingBuffer = head +
// markedText + tail. Unlike cursorIndex, which is UTF-8 based,
// markStartGridCursorIndex is in the same unit that a Gramambular's grid
// builder uses. In other words, markStartGridCursorIndex is the beginning
// position of the reading cursor. This makes it easy for a key handler to know
// where the marked range is when combined with the grid builder's (reading)
// cursor index.
struct Marking : NotEmpty {
  Marking(const std::string& buf, const size_t composingBufferCursorIndex,
          const std::string& tooltipText, const size_t startCursorIndexInGrid,
          std::string headText, std::string markedText, std::string tailText,
          std::string readingText, const bool canAccept)
      : NotEmpty(buf, composingBufferCursorIndex, tooltipText),
        markStartGridCursorIndex(startCursorIndexInGrid),
        head(std::move(headText)),
        markedText(std::move(markedText)),
        tail(std::move(tailText)),
        reading(std::move(readingText)),
        acceptable(canAccept) {}

  const size_t markStartGridCursorIndex;
  const std::string head;
  const std::string markedText;
  const std::string tail;
  const std::string reading;
  const bool acceptable;
};

}  // namespace InputStates

}  // namespace McBopomofo

#endif  // SRC_INPUTSTATE_H_
