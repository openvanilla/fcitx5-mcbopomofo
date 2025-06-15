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

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace McBopomofo {

enum class ChineseNumberStyle {
  LOWER,
  UPPER,
  SUZHOU,
};

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
  ~NotEmpty() override = default;

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

  Inputting(const Inputting& state)
      : NotEmpty(state.composingBuffer, state.cursorIndex, state.tooltip) {}
};

// Candidate selecting state with a non-empty composing buffer.
struct ChoosingCandidate : NotEmpty {
  struct Candidate;

  ChoosingCandidate(const std::string& buf, const size_t index,
                    const size_t originalIndex, std::vector<Candidate> cs)
      : NotEmpty(buf, index),
        candidates(std::move(cs)),
        originalCursor(originalIndex) {}

  ChoosingCandidate(const ChoosingCandidate& state)
      : NotEmpty(state.composingBuffer, state.cursorIndex),
        candidates(state.candidates),
        originalCursor(state.originalCursor) {}

  const std::vector<Candidate> candidates;
  size_t originalCursor;

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

  Marking(const Marking& state)
      : NotEmpty(state.composingBuffer, state.cursorIndex, state.tooltip),
        markStartGridCursorIndex(state.markStartGridCursorIndex),
        head(state.head),
        markedText(state.markedText),
        tail(state.tail),
        reading(state.reading),
        acceptable(state.acceptable) {}

  const size_t markStartGridCursorIndex;
  const std::string head;
  const std::string markedText;
  const std::string tail;
  const std::string reading;
  const bool acceptable;
};

struct SelectingDictionary : NotEmpty {
  SelectingDictionary(std::unique_ptr<NotEmpty> previousState,
                      std::string selectedPhrase, size_t selectedIndex,
                      std::vector<std::string> menu)
      : NotEmpty(previousState->composingBuffer, previousState->cursorIndex,
                 previousState->tooltip),
        previousState(std::move(previousState)),
        selectedPhrase(std::move(selectedPhrase)),
        selectedCandidateIndex(selectedIndex),
        menu(std::move(menu)) {}

  SelectingDictionary(const SelectingDictionary& state)
      : NotEmpty(state.previousState->composingBuffer,
                 state.previousState->cursorIndex,
                 state.previousState->tooltip),
        selectedPhrase(state.selectedPhrase),
        selectedCandidateIndex(state.selectedCandidateIndex),
        menu(state.menu) {
    auto* choosingCandidate =
        dynamic_cast<ChoosingCandidate*>(state.previousState.get());
    auto* marking = dynamic_cast<Marking*>(state.previousState.get());
    std::unique_ptr<NotEmpty> copy;

    if (choosingCandidate != nullptr) {
      copy = std::make_unique<ChoosingCandidate>(*choosingCandidate);
    } else if (marking != nullptr) {
      copy = std::make_unique<Marking>(*marking);
    }
    previousState = std::move(copy);
  }

  std::unique_ptr<NotEmpty> previousState;
  std::string selectedPhrase;
  size_t selectedCandidateIndex;
  std::vector<std::string> menu;
};

struct ShowingCharInfo : NotEmpty {
  ShowingCharInfo(std::unique_ptr<SelectingDictionary> previousState,
                  std::string selectedPhrase)
      : NotEmpty(previousState->previousState->composingBuffer,
                 previousState->previousState->cursorIndex,
                 previousState->previousState->tooltip),
        previousState(std::move(previousState)),
        selectedPhrase(std::move(selectedPhrase)) {}

  std::unique_ptr<SelectingDictionary> previousState;
  std::string selectedPhrase;
};

struct AssociatedPhrases : NotEmpty {
  AssociatedPhrases(std::unique_ptr<NotEmpty> prevState, size_t pfxCursorIndex,
                    std::string pfxReading, std::string pfxValue,
                    size_t selIndex,
                    std::vector<ChoosingCandidate::Candidate> cs,
                    bool useShiftKey = false)
      : NotEmpty(prevState->composingBuffer, prevState->cursorIndex,
                 prevState->tooltip),
        previousState(std::move(prevState)),
        prefixCursorIndex(pfxCursorIndex),
        prefixReading(std::move(pfxReading)),
        prefixValue(std::move(pfxValue)),
        selectedCandidateIndex(selIndex),
        candidates(std::move(cs)),
        useShiftKey(useShiftKey) {}
  std::unique_ptr<NotEmpty> previousState;
  size_t prefixCursorIndex;
  std::string prefixReading;
  std::string prefixValue;
  size_t selectedCandidateIndex;
  const std::vector<ChoosingCandidate::Candidate> candidates;
  bool useShiftKey;
};

struct AssociatedPhrasesPlain : InputState {
  explicit AssociatedPhrasesPlain(std::vector<ChoosingCandidate::Candidate> cs)
      : candidates(std::move(cs)) {}
  const std::vector<ChoosingCandidate::Candidate> candidates;
};

struct EnclosingNumber : InputState {
  EnclosingNumber(std::string number = "") : number(std::move(number)) {}
  EnclosingNumber(EnclosingNumber const& number) : number(number.number) {}
  std::string composingBuffer() const { return "[標題數字] " + number; }
  std::string number;
};

struct ChineseNumber : InputState {
  ChineseNumber(std::string number, ChineseNumberStyle style)
      : number(std::move(number)), style(style) {}
  ChineseNumber(ChineseNumber const& number)
      : number(number.number), style(number.style) {}

  std::string composingBuffer() const {
    if (style == ChineseNumberStyle::LOWER) {
      return "[中文數字] " + number;
    } else if (style == ChineseNumberStyle::UPPER) {
      return "[大寫數字] " + number;
    } else if (style == ChineseNumberStyle::SUZHOU) {
      return "[蘇州碼] " + number;
    }
    return number;
  }

  std::string number;
  ChineseNumberStyle style;
};

struct SelectingDateMacro : InputState {
  explicit SelectingDateMacro(
      const std::function<std::string(std::string)>& converter);

  std::vector<std::string> menu;
};

struct SelectingFeature : InputState {
  struct Feature {
    Feature(std::string name,
            std::function<std::unique_ptr<InputState>(void)> nextState)
        : name(std::move(name)), nextState(std::move(nextState)) {}
    std::string name;
    std::function<std::unique_ptr<InputState>(void)> nextState;
  };

  explicit SelectingFeature(std::function<std::string(std::string)> converter)
      : converter(std::move(converter)) {
    features.emplace_back("日期與時間", [this]() {
      return std::make_unique<SelectingDateMacro>(this->converter);
    });
    features.emplace_back("標題數字",
                          []() { return std::make_unique<EnclosingNumber>(); });
    features.emplace_back("中文數字", []() {
      return std::make_unique<ChineseNumber>("", ChineseNumberStyle::LOWER);
    });
    features.emplace_back("大寫數字", []() {
      return std::make_unique<ChineseNumber>("", ChineseNumberStyle::UPPER);
    });
    features.emplace_back("蘇州碼", []() {
      return std::make_unique<ChineseNumber>("", ChineseNumberStyle::SUZHOU);
    });
  }

  std::unique_ptr<InputState> nextState(size_t index) {
    return features[index].nextState();
  }

  std::vector<Feature> features;

 protected:
  std::function<std::string(std::string)> converter;
};

struct CustomMenu : NotEmpty {
  struct MenuEntry {
    MenuEntry(std::string name, std::function<void(void)> callback)
        : name(std::move(name)), callback(std::move(callback)) {}
    std::string name;
    std::function<void(void)> callback;
  };

  explicit CustomMenu(std::unique_ptr<NotEmpty> previousState,
                      std::string title, std::vector<MenuEntry> entries)
      : NotEmpty(previousState->composingBuffer, previousState->cursorIndex,
                 title),
        previousState(std::move(previousState)),
        entries(std::move(entries)) {}

  std::unique_ptr<NotEmpty> previousState;
  std::vector<MenuEntry> entries;
};

}  // namespace InputStates

}  // namespace McBopomofo

#endif  // SRC_INPUTSTATE_H_
