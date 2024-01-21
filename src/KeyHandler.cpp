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

#include "KeyHandler.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include "Log.h"
#include "UTF8Helper.h"

namespace McBopomofo {

constexpr char kSpaceSeparator[] = " ";
constexpr char kPunctuationListKey = '`';  // Hit the key to bring up the list.
constexpr char kPunctuationListUnigramKey[] = "_punctuation_list";
constexpr char kPunctuationKeyPrefix[] = "_punctuation_";
constexpr char kCtrlPunctuationKeyPrefix[] = "_ctrl_punctuation_";
constexpr char kLetterPrefix[] = "_letter_";
constexpr size_t kMinValidMarkingReadingCount = 2;
constexpr size_t kMaxValidMarkingReadingCount = 6;

constexpr int kUserOverrideModelCapacity = 500;
constexpr double kObservedOverrideHalfLife = 5400.0;  // 1.5 hr.
// Unigram whose score is below this shouldn't be put into user override model.
constexpr double kNoOverrideThreshold = -8.0;

static const char* GetKeyboardLayoutName(
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
  // NOLINTBEGIN(readability-else-after-return)
  if (layout == Formosa::Mandarin::BopomofoKeyboardLayout::ETenLayout()) {
    return "ETen";
  } else if (layout == Formosa::Mandarin::BopomofoKeyboardLayout::HsuLayout()) {
    return "Hsu";
  } else if (layout ==
             Formosa::Mandarin::BopomofoKeyboardLayout::ETen26Layout()) {
    return "ETen26";
  } else if (layout ==
             Formosa::Mandarin::BopomofoKeyboardLayout::HanyuPinyinLayout()) {
    return "HanyuPinyin";
  } else if (layout == Formosa::Mandarin::BopomofoKeyboardLayout::IBMLayout()) {
    return "IBM";
  }
  return "Standard";
  // NOLINTEND(readability-else-after-return)
}

static bool MarkedPhraseExists(
    const std::shared_ptr<Formosa::Gramambular2::LanguageModel>& lm,
    const std::string& reading, const std::string& value) {
  if (!lm->hasUnigrams(reading)) {
    return false;
  }

  auto unigrams = lm->getUnigrams(reading);
  return std::any_of(
      unigrams.begin(), unigrams.end(),
      [&value](const auto& unigram) { return unigram.value() == value; });
}

static double GetEpochNowInSeconds() {
  auto now = std::chrono::system_clock::now();
  int64_t timestamp = std::chrono::time_point_cast<std::chrono::seconds>(now)
                          .time_since_epoch()
                          .count();
  return static_cast<double>(timestamp);
}

KeyHandler::KeyHandler(
    std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel,
    std::shared_ptr<UserPhraseAdder> userPhraseAdder,
    std::unique_ptr<LocalizedStrings> localizedStrings)
    : lm_(std::move(languageModel)),
      grid_(lm_),
      userPhraseAdder_(std::move(userPhraseAdder)),
      localizedStrings_(std::move(localizedStrings)),
      userOverrideModel_(kUserOverrideModelCapacity, kObservedOverrideHalfLife),
      reading_(Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout()) {
  dictionaryServices_ = std::make_unique<DictionaryServices>();
  dictionaryServices_->load();
}

bool KeyHandler::handle(Key key, McBopomofo::InputState* state,
                        StateCallback stateCallback,
                        ErrorCallback errorCallback) {
  // From Key's definition, if shiftPressed is true, it can't be a simple key
  // that can be represented by ASCII.
  char simpleAscii = (key.ctrlPressed || key.shiftPressed) ? '\0' : key.ascii;

  // See if it's valid BPMF reading.
  bool keyConsumedByReading = false;
  if (reading_.isValidKey(simpleAscii)) {
    reading_.combineKey(simpleAscii);
    keyConsumedByReading = true;
    // If asciiChar does not lead to a tone marker, we are done. Tone marker
    // would lead to composing of the reading, which is handled after this.
    if (!reading_.hasToneMarker()) {
      stateCallback(buildInputtingState());
      return true;
    }
  }

  // Compose the reading if either there's a tone marker, or if the reading is
  // not empty, and space is pressed.
  bool shouldComposeReading =
      (reading_.hasToneMarker() && !reading_.hasToneMarkerOnly()) ||
      (!reading_.isEmpty() && simpleAscii == Key::SPACE);

  if (shouldComposeReading) {
    std::string syllable = reading_.syllable().composedString();
    reading_.clear();

    if (!lm_->hasUnigrams(syllable)) {
      errorCallback();
      if (grid_.length() == 0) {
        stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      } else {
        stateCallback(buildInputtingState());
      }
      return true;
    }

    grid_.insertReading(syllable);
    walk();

    if (inputMode_ != McBopomofo::InputMode::PlainBopomofo) {
      UserOverrideModel::Suggestion suggestion = userOverrideModel_.suggest(
          latestWalk_, actualCandidateCursorIndex(), GetEpochNowInSeconds());

      if (!suggestion.empty()) {
        Formosa::Gramambular2::ReadingGrid::Node::OverrideType t =
            suggestion.forceHighScoreOverride
                ? Formosa::Gramambular2::ReadingGrid::Node::OverrideType::
                      kOverrideValueWithHighScore
                : Formosa::Gramambular2::ReadingGrid::Node::OverrideType::
                      kOverrideValueWithScoreFromTopUnigram;
        grid_.overrideCandidate(actualCandidateCursorIndex(),
                                suggestion.candidate, t);
        walk();
      }
    }

    if (inputMode_ == McBopomofo::InputMode::PlainBopomofo) {
      auto inputting = buildInputtingState();
      auto choosingCandidate = buildChoosingCandidateState(inputting.get());
      if (choosingCandidate->candidates.size() == 1) {
        reset();
        std::string value = choosingCandidate->candidates[0].value;
        auto committingState = std::make_unique<InputStates::Committing>(value);
        stateCallback(std::move(committingState));

        if (associatedPhrasesEnabled_) {
          auto associatedPhrasesPlainState =
              buildAssociatedPhrasesPlainState(value);
          if (associatedPhrasesPlainState != nullptr) {
            stateCallback(std::move(associatedPhrasesPlainState));
          }
        }
      } else {
        stateCallback(std::move(choosingCandidate));
      }
    } else {
      auto inputting = buildInputtingState();
      stateCallback(std::move(inputting));
    }
    return true;
  }

  // The only possibility for this to be true is that the Bopomofo reading
  // already has a tone marker but the last key is *not* a tone marker key. An
  // example is the sequence "6u" with the Standard layout, which produces "ㄧˊ"
  // but does not compose. Only sequences such as "u6", "6u6", "6u3", or "6u "
  // would compose.
  if (keyConsumedByReading) {
    stateCallback(buildInputtingState());
    return true;
  }

  // Shift + Space.
  if (key.ascii == Key::SPACE && key.shiftPressed) {
    if (putLowercaseLettersToComposingBuffer_) {
      grid_.insertReading(" ");
      walk();
      stateCallback(buildInputtingState());
    } else {
      if (grid_.length() != 0) {
        auto inputtingState = buildInputtingState();
        // Steal the composingBuffer built by the inputting state.
        auto committingState = std::make_unique<InputStates::Committing>(
            inputtingState->composingBuffer);
        stateCallback(std::move(committingState));
      }
      auto committingState = std::make_unique<InputStates::Committing>(" ");
      stateCallback(std::move(committingState));
      reset();
    }
    return true;
  }

  // Space hit: see if we should enter the candidate choosing state.
  auto* maybeNotEmptyState = dynamic_cast<InputStates::NotEmpty*>(state);
  if ((simpleAscii == Key::SPACE || key.name == Key::KeyName::DOWN) &&
      maybeNotEmptyState != nullptr && reading_.isEmpty()) {
    stateCallback(buildChoosingCandidateState(maybeNotEmptyState));
    return true;
  }

  // Esc hit.
  if (simpleAscii == Key::ESC) {
    if (maybeNotEmptyState == nullptr) {
      return false;
    }

    if (escKeyClearsEntireComposingBuffer_) {
      reset();
      stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      return true;
    }

    if (!reading_.isEmpty()) {
      reading_.clear();
      if (grid_.length() == 0) {
        stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      } else {
        stateCallback(buildInputtingState());
      }
    } else {
      stateCallback(buildInputtingState());
    }
    return true;
  }

  // Tab key.
  if (key.ascii == Key::TAB) {
    return handleTabKey(key, state, stateCallback, errorCallback);
  }

  // Cursor keys.
  if (key.isCursorKeys()) {
    return handleCursorKeys(key, state, stateCallback, errorCallback);
  }

  // Backspace and Del.
  if (key.isDeleteKeys()) {
    return handleDeleteKeys(key, state, stateCallback, errorCallback);
  }

  // Enter.
  if (key.ascii == Key::RETURN) {
    if (maybeNotEmptyState == nullptr) {
      return false;
    }

    if (!reading_.isEmpty()) {
      errorCallback();
      stateCallback(buildInputtingState());
      return true;
    }

    // Shift + Enter
    if (key.shiftPressed && inputMode_ == InputMode::McBopomofo &&
        associatedPhrasesEnabled_) {
      size_t cursor = grid_.cursor();
      if (cursor < 1) {
        errorCallback();
        return true;
      }
      auto inputting = dynamic_cast<InputStates::Inputting*>(state);
      if (inputting != nullptr) {
        std::string composerBuffer = inputting->composingBuffer;
        std::string characterBeforeCursor =
            GetCodePoint(composerBuffer, cursor - 1);
        auto candidates = grid_.candidatesAt(cursor - 1);
        auto candidateIt =
            std::find_if(candidates.begin(), candidates.end(),
                         [characterBeforeCursor](auto candidate) {
                           return candidate.value == characterBeforeCursor;
                         });

        if (candidateIt == candidates.end()) {
          // The character before the cursor is not composed by a single
          // reading.
          errorCallback();
          return true;
        }

        std::optional<Formosa::Gramambular2::ReadingGrid::NodePtr>
            oneUnitLongSpan = grid_.findInSpan(
                cursor - 1,
                [](const Formosa::Gramambular2::ReadingGrid::NodePtr& node) {
                  return node->spanningLength() == 1;
                });

        if (!oneUnitLongSpan.has_value()) {
          errorCallback();
          return true;
        }
        std::string reading = (*oneUnitLongSpan)->reading();
        auto associatedPhrasesState = buildAssociatedPhrasesState(
            buildInputtingState(), characterBeforeCursor, reading, 0);
        stateCallback(std::move(associatedPhrasesState));
      }
      return true;
    }

    // Ctrl + Enter
    if (key.ctrlPressed && inputMode_ == InputMode::McBopomofo) {
      if (ctrlEnterKey_ == KeyHandlerCtrlEnter::OutputBpmfReadings) {
        std::vector<std::string> readings = grid_.readings();
        std::string readingValue;
        for (auto it = readings.begin(); it != readings.end(); ++it) {
          readingValue += *it;
          if (it + 1 != readings.end()) {
            readingValue += kJoinSeparator;
          }
        }

        auto committingState =
            std::make_unique<InputStates::Committing>(readingValue);
        stateCallback(std::move(committingState));
        reset();
        return true;
      }
      if (ctrlEnterKey_ == KeyHandlerCtrlEnter::OutputHTMLRubyText) {
        auto output = getHTMLRubyText();
        auto committingState =
            std::make_unique<InputStates::Committing>(output);
        stateCallback(std::move(committingState));
        reset();
        return true;
      }
      return false;
    }

    // See if we are in Marking state, and, if a valid mark, accept it.
    if (auto* marking = dynamic_cast<InputStates::Marking*>(state)) {
      if (marking->acceptable) {
        userPhraseAdder_->addUserPhrase(marking->reading, marking->markedText);
        onAddNewPhrase_(marking->markedText);

        // If the cursor was at the end of the buffer when the marking started,
        // move back.
        if (marking->markStartGridCursorIndex == grid_.length() &&
            grid_.cursor() < marking->markStartGridCursorIndex) {
          grid_.setCursor(grid_.length());
        }

        stateCallback(buildInputtingState());
      } else {
        errorCallback();
        stateCallback(buildMarkingState(marking->markStartGridCursorIndex));
      }
      return true;
    }

    auto inputtingState = buildInputtingState();
    // Steal the composingBuffer built by the inputting state.
    auto committingState = std::make_unique<InputStates::Committing>(
        inputtingState->composingBuffer);
    stateCallback(std::move(committingState));
    reset();
    return true;
  }

  // Question key
  if (simpleAscii == '?') {
    auto* marking = dynamic_cast<InputStates::Marking*>(state);
    if (marking != nullptr) {
      // Enter the state to select a dictionary service.
      std::string markedText = marking->markedText;
      auto copy = std::make_unique<InputStates::Marking>(*marking);
      auto selecting =
          buildSelectingDictionaryState(std::move(copy), markedText, 0);
      stateCallback(std::move(selecting));
      return true;
    }
  }

  // Punctuation key: backtick or grave accent.
  if (simpleAscii == kPunctuationListKey &&
      lm_->hasUnigrams(kPunctuationListUnigramKey)) {
    if (reading_.isEmpty()) {
      grid_.insertReading(kPunctuationListUnigramKey);
      walk();

      auto inputtingState = buildInputtingState();
      auto choosingCandidateState =
          buildChoosingCandidateState(inputtingState.get());
      stateCallback(std::move(inputtingState));
      stateCallback(std::move(choosingCandidateState));
    } else {
      // Punctuation ignored if a bopomofo reading is active..
      errorCallback();
    }
    return true;
  }

  if (key.ascii != 0) {
    std::string chrStr(1, key.ascii);
    std::string unigram;

    if (key.ctrlPressed) {
      unigram = std::string(kCtrlPunctuationKeyPrefix) + chrStr;
      return handlePunctuation(unigram, stateCallback, errorCallback);
    }

    // Bopomofo layout-specific punctuation handling.
    unigram = std::string(kPunctuationKeyPrefix) +
              GetKeyboardLayoutName(reading_.keyboardLayout()) + "_" + chrStr;
    if (handlePunctuation(unigram, stateCallback, errorCallback)) {
      return true;
    }

    // Not handled, try generic punctuations.
    unigram = std::string(kPunctuationKeyPrefix) + chrStr;
    if (handlePunctuation(unigram, stateCallback, errorCallback)) {
      return true;
    }

    // Upper case letters.
    if (simpleAscii >= 'A' && simpleAscii <= 'Z') {
      if (putLowercaseLettersToComposingBuffer_) {
        unigram = std::string(kLetterPrefix) + chrStr;

        // Ignore return value, since we always return true below.
        handlePunctuation(unigram, stateCallback, errorCallback);
      } else {
        // If current state is *not* NonEmpty, it must be Empty.
        if (maybeNotEmptyState == nullptr) {
          // We don't need to handle this key.
          return false;
        }

        // First, commit what's already in the composing buffer.
        auto inputtingState = buildInputtingState();
        // Steal the composingBuffer built by the inputting state.
        auto committingState = std::make_unique<InputStates::Committing>(
            inputtingState->composingBuffer);
        stateCallback(std::move(committingState));

        // Then we commit that single character.
        stateCallback(std::make_unique<InputStates::Committing>(chrStr));
        reset();
      }
      return true;
    }
  }

  // No key is handled. Refresh and consume the key.
  if (maybeNotEmptyState != nullptr) {
    // It is possible that FCITX just passes a single shift key event here.
    // When it is in the marking state, we do not want to go back to the
    // inputting state anyway.
    auto* marking = dynamic_cast<InputStates::Marking*>(state);
    if (marking != nullptr) {
      return true;
    }
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  return false;
}

void KeyHandler::candidateSelected(
    const InputStates::ChoosingCandidate::Candidate& candidate,
    StateCallback stateCallback) {
  if (inputMode_ == InputMode::PlainBopomofo) {
    reset();
    std::unique_ptr<InputStates::Committing> committingState =
        std::make_unique<InputStates::Committing>(candidate.value);
    stateCallback(std::move(committingState));

    if (associatedPhrasesEnabled_) {
      auto associatedPhrasesPlainState =
          buildAssociatedPhrasesPlainState(candidate.value);
      if (associatedPhrasesPlainState != nullptr) {
        stateCallback(std::move(associatedPhrasesPlainState));
      }
    }
    return;
  }

  pinNode(candidate);
  stateCallback(buildInputtingState());
}

void KeyHandler::candidateAssociatedPhraseSelected(
    size_t index, const InputStates::ChoosingCandidate::Candidate& candidate,
    const std::string& phrase, const StateCallback& stateCallback) {
  pinNode(index, phrase, candidate.value);
  stateCallback(buildInputtingState());
}

void KeyHandler::dictionaryServiceSelected(std::string phrase, size_t index,
                                           InputState* currentState,
                                           StateCallback stateCallback) {
  dictionaryServices_->lookup(std::move(phrase), index, currentState,
                              stateCallback);
}

void KeyHandler::candidatePanelCancelled(StateCallback stateCallback) {
  if (inputMode_ == InputMode::PlainBopomofo) {
    reset();
    std::unique_ptr<InputStates::EmptyIgnoringPrevious>
        emptyIgnorePreviousState =
            std::make_unique<InputStates::EmptyIgnoringPrevious>();
    stateCallback(std::move(emptyIgnorePreviousState));
    return;
  }

  stateCallback(buildInputtingState());
}

bool KeyHandler::handleCandidateKeyForTraditionalBopomofoIfRequired(
    Key key, SelectCurrentCandidateCallback SelectCurrentCandidateCallback,
    StateCallback stateCallback, ErrorCallback errorCallback) {
  if (inputMode_ != McBopomofo::InputMode::PlainBopomofo) {
    return false;
  }

  const char chrStr = key.ascii;

  std::string customPunctuation =
      std::string(kPunctuationKeyPrefix) +
      GetKeyboardLayoutName(reading_.keyboardLayout()) + "_" +
      std::string(1, key.ascii);

  std::string punctuation = kPunctuationKeyPrefix + std::string(1, key.ascii);
  bool shouldAutoSelectCandidate = reading_.isValidKey(chrStr) ||
                                   lm_->hasUnigrams(customPunctuation) ||
                                   lm_->hasUnigrams(punctuation);
  if (!shouldAutoSelectCandidate) {
    if (chrStr >= 'A' && chrStr <= 'Z') {
      std::string letter = std::string(kLetterPrefix) + chrStr;
      if (lm_->hasUnigrams(letter)) {
        shouldAutoSelectCandidate = true;
      }
    }
  }

  if (shouldAutoSelectCandidate) {
    SelectCurrentCandidateCallback();
    reset();
    handle(key, std::make_unique<InputStates::Empty>().get(), stateCallback,
           errorCallback);
    return true;
  }
  return false;
}

void KeyHandler::reset() {
  reading_.clear();
  grid_.clear();
  latestWalk_ = Formosa::Gramambular2::ReadingGrid::WalkResult();
}

#pragma region Settings

McBopomofo::InputMode KeyHandler::inputMode() { return inputMode_; }

void KeyHandler::setInputMode(McBopomofo::InputMode mode) { inputMode_ = mode; }

void KeyHandler::setKeyboardLayout(
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
  reading_.setKeyboardLayout(layout);
}

void KeyHandler::setSelectPhraseAfterCursorAsCandidate(bool flag) {
  selectPhraseAfterCursorAsCandidate_ = flag;
}

void KeyHandler::setMoveCursorAfterSelection(bool flag) {
  moveCursorAfterSelection_ = flag;
}

void KeyHandler::setPutLowercaseLettersToComposingBuffer(bool flag) {
  putLowercaseLettersToComposingBuffer_ = flag;
}

void KeyHandler::setEscKeyClearsEntireComposingBuffer(bool flag) {
  escKeyClearsEntireComposingBuffer_ = flag;
}

void KeyHandler::setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior) {
  ctrlEnterKey_ = behavior;
}

void KeyHandler::setAssociatedPhrasesEnabled(bool enabled) {
  associatedPhrasesEnabled_ = enabled;
}

void KeyHandler::setOnAddNewPhrase(
    std::function<void(const std::string&)> onAddNewPhrase) {
  onAddNewPhrase_ = std::move(onAddNewPhrase);
}

#pragma endregion Settings

#pragma region Key_Handling

bool KeyHandler::handleTabKey(Key key, McBopomofo::InputState* state,
                              const StateCallback& stateCallback,
                              const ErrorCallback& errorCallback) {
  if (reading_.isEmpty() && latestWalk_.nodes.empty()) {
    return false;
  }

  auto* inputting = dynamic_cast<InputStates::Inputting*>(state);

  if (inputting == nullptr) {
    errorCallback();
    return true;
  }

  if (!reading_.isEmpty()) {
    errorCallback();
    return true;
  }

  const auto candidates = buildChoosingCandidateState(inputting)->candidates;
  if (candidates.empty()) {
    errorCallback();
    return true;
  }

  auto nodeIter = latestWalk_.findNodeAt(actualCandidateCursorIndex());
  if (nodeIter == latestWalk_.nodes.cend()) {
    // Shouldn't happen.
    errorCallback();
    return true;
  }
  const Formosa::Gramambular2::ReadingGrid::NodePtr& currentNode = *nodeIter;

  size_t currentIndex = 0;
  if (!currentNode->isOverridden()) {
    // If the user never selects a candidate for the node, we start from the
    // first candidate, so the user has a chance to use the unigram with two or
    // more characters when type the tab key for the first time.
    //
    // In other words, if a user type two BPMF readings, but the score of seeing
    // them as two unigrams is higher than a phrase with two characters, the
    // user can just use the longer phrase by typing the tab key.
    if (candidates[0].reading == currentNode->reading() &&
        candidates[0].value == currentNode->value()) {
      // If the first candidate is the value of the current node, we use next
      // one.
      if (key.shiftPressed) {
        currentIndex = candidates.size() - 1;
      } else {
        currentIndex = 1;
      }
    }
  } else {
    for (const auto& candidate : candidates) {
      if (candidate.reading == currentNode->reading() &&
          candidate.value == currentNode->value()) {
        if (key.shiftPressed) {
          currentIndex == 0 ? currentIndex = candidates.size() - 1
                            : currentIndex--;
        } else {
          currentIndex++;
        }
        break;
      }
      currentIndex++;
    }
  }

  if (currentIndex >= candidates.size()) {
    currentIndex = 0;
  }

  pinNode(candidates[currentIndex],
          /*useMoveCursorAfterSelectionSetting=*/false);
  stateCallback(buildInputtingState());
  return true;
}

bool KeyHandler::handleCursorKeys(Key key, McBopomofo::InputState* state,
                                  const StateCallback& stateCallback,
                                  const ErrorCallback& errorCallback) {
  if (dynamic_cast<InputStates::Inputting*>(state) == nullptr &&
      dynamic_cast<InputStates::Marking*>(state) == nullptr) {
    return false;
  }
  size_t markBeginCursorIndex = grid_.cursor();
  auto* marking = dynamic_cast<InputStates::Marking*>(state);
  if (marking != nullptr) {
    markBeginCursorIndex = marking->markStartGridCursorIndex;
  }

  if (!reading_.isEmpty()) {
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  bool isValidMove = false;
  switch (key.name) {
    case Key::KeyName::LEFT:
      if (grid_.cursor() > 0) {
        grid_.setCursor(grid_.cursor() - 1);
        isValidMove = true;
      }
      break;
    case Key::KeyName::RIGHT:
      if (grid_.cursor() < grid_.length()) {
        grid_.setCursor(grid_.cursor() + 1);
        isValidMove = true;
      }
      break;
    case Key::KeyName::HOME:
      grid_.setCursor(0);
      isValidMove = true;
      break;
    case Key::KeyName::END:
      grid_.setCursor(grid_.length());
      isValidMove = true;
      break;
    default:
      // Ignored.
      break;
  }

  if (!isValidMove) {
    errorCallback();
  }

  if (key.shiftPressed && grid_.cursor() != markBeginCursorIndex) {
    stateCallback(buildMarkingState(markBeginCursorIndex));
  } else {
    stateCallback(buildInputtingState());
  }
  return true;
}

bool KeyHandler::handleDeleteKeys(Key key, McBopomofo::InputState* state,
                                  const StateCallback& stateCallback,
                                  const ErrorCallback& errorCallback) {
  if (dynamic_cast<InputStates::NotEmpty*>(state) == nullptr) {
    return false;
  }

  if (reading_.hasToneMarkerOnly()) {
    reading_.clear();
  } else if (reading_.isEmpty()) {
    bool isValidDelete = false;

    if (key.ascii == Key::BACKSPACE && grid_.cursor() > 0) {
      grid_.deleteReadingBeforeCursor();
      isValidDelete = true;
    } else if (key.ascii == Key::DELETE && grid_.cursor() < grid_.length()) {
      grid_.deleteReadingAfterCursor();
      isValidDelete = true;
    }
    if (!isValidDelete) {
      errorCallback();
      stateCallback(buildInputtingState());
      return true;
    }
    walk();
  } else {
    if (key.ascii == Key::BACKSPACE) {
      reading_.backspace();
    } else {
      // Del not supported when bopomofo reading is active.
      errorCallback();
    }
  }

  if (reading_.isEmpty() && grid_.length() == 0) {
    // Cancel the previous input state if everything is empty now.
    stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
  } else {
    stateCallback(buildInputtingState());
  }
  return true;
}

bool KeyHandler::handlePunctuation(const std::string& punctuationUnigramKey,
                                   const StateCallback& stateCallback,
                                   const ErrorCallback& errorCallback) {
  if (!lm_->hasUnigrams(punctuationUnigramKey)) {
    return false;
  }

  if (!reading_.isEmpty()) {
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  grid_.insertReading(punctuationUnigramKey);
  walk();

  if (inputMode_ == McBopomofo::InputMode::PlainBopomofo) {
    auto inputting = buildInputtingState();
    auto choosingCandidate = buildChoosingCandidateState(inputting.get());
    if (choosingCandidate->candidates.size() == 1) {
      reset();
      std::string value = choosingCandidate->candidates[0].value;
      auto committingState = std::make_unique<InputStates::Committing>(value);
      stateCallback(std::move(committingState));
    } else {
      stateCallback(std::move(choosingCandidate));
    }
  } else {
    auto inputting = buildInputtingState();
    stateCallback(std::move(inputting));
  }

  return true;
}

#pragma endregion Key_Handling

#pragma region Output

std::string KeyHandler::getHTMLRubyText() {
  std::string composed;
  for (const auto& node : latestWalk_.nodes) {
    std::string key = node->reading();
    std::replace(key.begin(), key.end(), kJoinSeparator[0], kSpaceSeparator[0]);
    std::string value = node->value();

    // If a key starts with underscore, it is usually for a punctuation or a
    // symbol but not a Bopomofo reading, so we just ignore such case.
    if (key.rfind(std::string("_"), 0) == 0) {
      composed += value;
    } else {
      composed += "<ruby>";
      composed += value;
      composed += "<rp>(</rp><rt>" + key + "</rt><rp>)</rp>";
      composed += "</ruby>";
    }
  }
  return composed;
}

KeyHandler::ComposedString KeyHandler::getComposedString(size_t builderCursor) {
  // To construct an Inputting state, we need to first retrieve the entire
  // composing buffer from the current grid, then split the composed string
  // into head and tail, so that we can insert the current reading (if
  // not-empty) between them.
  //
  // We'll also need to compute the UTF-8 cursor index. The idea here is we
  // use a "running" index_ that will eventually catch the cursor index_ in the
  // builder. The tricky part is that if the spanning length of the node that
  // the cursor is at does not agree with the actual codepoint count of the
  // node's value, we'll need to move the cursor at the end of the node to
  // avoid confusions.

  size_t runningCursor = 0;  // spanning-length-based, like the builder cursor

  std::string composed;
  size_t composedCursor =
      0;  // UTF-8 (so "byte") cursor per fcitx5 requirement.

  std::string tooltip;

  for (const auto& node : latestWalk_.nodes) {
    std::string value = node->value();
    composed += value;

    // No work if runningCursor has already caught up with builderCursor.
    if (runningCursor == builderCursor) {
      continue;
    }
    size_t readingLength = node->spanningLength();

    // Simple case: if the running cursor is behind, add the spanning length.
    if (runningCursor + readingLength <= builderCursor) {
      composedCursor += value.length();
      runningCursor += readingLength;
      continue;
    }

    // The builder cursor is in the middle of the node.
    size_t distance = builderCursor - runningCursor;
    size_t valueCodePointCount = CodePointCount(value);

    // The actual partial value's code point length is the shorter of the
    // distance and the value's code point count.
    size_t cpLen = std::min(distance, valueCodePointCount);
    std::string actualValue = SubstringToCodePoints(value, cpLen);
    composedCursor += actualValue.length();
    runningCursor += distance;

    // Create a tooltip to warn the user that their cursor is between two
    // readings (syllables) even if the cursor is not in the middle of a
    // composed string due to its being shorter than the number of readings.
    if (valueCodePointCount < readingLength) {
      // builderCursor is guaranteed to be > 0. If it was 0, we wouldn't even
      // reach here due to runningCursor having already "caught up" with
      // builderCursor. It is also guaranteed to be less than the size of the
      // builder's readings for the same reason: runningCursor would have
      // already caught up.
      const std::string& prevReading = grid_.readings()[builderCursor - 1];
      const std::string& nextReading = grid_.readings()[builderCursor];

      tooltip =
          localizedStrings_->cursorIsBetweenSyllables(prevReading, nextReading);
    }
  }

  std::string head = composed.substr(0, composedCursor);
  std::string tail =
      composed.substr(composedCursor, composed.length() - composedCursor);
  return KeyHandler::ComposedString{
      .head = head, .tail = tail, .tooltip = tooltip};
}

#pragma endregion Output

#pragma region Build_States

std::unique_ptr<InputStates::Inputting> KeyHandler::buildInputtingState() {
  auto composedString = getComposedString(grid_.cursor());

  std::string head = composedString.head;
  std::string reading = reading_.composedString();
  std::string tail = composedString.tail;

  std::string composingBuffer = head + reading + tail;
  size_t cursorIndex = head.length() + reading.length();
  return std::make_unique<InputStates::Inputting>(composingBuffer, cursorIndex,
                                                  composedString.tooltip);
}

std::unique_ptr<InputStates::ChoosingCandidate>
KeyHandler::buildChoosingCandidateState(InputStates::NotEmpty* nonEmptyState) {
  auto candidates = grid_.candidatesAt(actualCandidateCursorIndex());
  std::vector<InputStates::ChoosingCandidate::Candidate> stateCandidates;
  for (const auto& c : candidates) {
    stateCandidates.emplace_back(c.reading, c.value);
  }

  return std::make_unique<InputStates::ChoosingCandidate>(
      nonEmptyState->composingBuffer, nonEmptyState->cursorIndex,
      std::move(stateCandidates));
}

std::unique_ptr<InputStates::Marking> KeyHandler::buildMarkingState(
    size_t beginCursorIndex) {
  // We simply build two composed strings and use the delta between the
  // shorter and the longer one as the marked text.
  ComposedString from = getComposedString(beginCursorIndex);
  ComposedString to = getComposedString(grid_.cursor());
  size_t composedStringCursorIndex = to.head.length();
  std::string composed = to.head + to.tail;
  size_t fromIndex = beginCursorIndex;
  size_t toIndex = grid_.cursor();

  if (beginCursorIndex > grid_.cursor()) {
    std::swap(from, to);
    std::swap(fromIndex, toIndex);
  }

  // Now from is shorter and to is longer. The marked text is just the delta.
  std::string head = from.head;
  std::string marked =
      std::string(to.head.begin() + static_cast<ptrdiff_t>(from.head.length()),
                  to.head.end());
  std::string tail = to.tail;

  // Collect the readings.
  auto readingBegin = grid_.readings().begin();
  std::vector<std::string> readings(
      readingBegin + static_cast<ptrdiff_t>(fromIndex),
      readingBegin + static_cast<ptrdiff_t>(toIndex));
  std::string readingUiText;  // What the user sees.
  std::string readingValue;   // What is used for adding a user phrase.
  for (auto it = readings.begin(); it != readings.end(); ++it) {
    readingValue += *it;
    readingUiText += *it;
    if (it + 1 != readings.end()) {
      readingValue += kJoinSeparator;
      readingUiText += " ";
    }
  }

  bool isValid = false;
  std::string status;
  // Validate the marking.
  if (readings.size() < kMinValidMarkingReadingCount) {
    status = localizedStrings_->syllablesRequired(kMinValidMarkingReadingCount);
  } else if (readings.size() > kMaxValidMarkingReadingCount) {
    status = localizedStrings_->syllablesMaximum(kMaxValidMarkingReadingCount);
  } else if (MarkedPhraseExists(lm_, readingValue, marked)) {
    status = localizedStrings_->phraseAlreadyExists();
  } else {
    status = localizedStrings_->pressEnterToAddThePhrase();
    isValid = true;
  }

  std::string tooltip = localizedStrings_->markedWithSyllablesAndStatus(
      marked, readingUiText, status);
  return std::make_unique<InputStates::Marking>(
      composed, composedStringCursorIndex, tooltip, beginCursorIndex, head,
      marked, tail, readingValue, isValid);
}

std::unique_ptr<InputStates::AssociatedPhrases>
KeyHandler::buildAssociatedPhrasesState(
    std::unique_ptr<InputStates::NotEmpty> previousState,
    const std::string& selectedPhrase, const std::string& selectedReading,
    size_t selectedCandidateIndex) {
  McBopomofoLM* lm = dynamic_cast<McBopomofoLM*>(lm_.get());

  if (lm == nullptr) {
    return nullptr;
  }

  if (lm->hasAssociatedPhrasesForKey(selectedPhrase)) {
    std::vector<std::string> phrases =
        lm->associatedPhrasesForKey(selectedPhrase);
    std::vector<InputStates::ChoosingCandidate::Candidate> cs;
    for (const auto& phrase : phrases) {
      cs.emplace_back(phrase, phrase);
    }
    return std::make_unique<InputStates::AssociatedPhrases>(
        std::move(previousState), selectedPhrase, selectedReading,
        selectedCandidateIndex, cs);
  }
  return nullptr;
}

std::unique_ptr<InputStates::AssociatedPhrasesPlain>
KeyHandler::buildAssociatedPhrasesPlainState(const std::string& key) {
  McBopomofoLM* lm = dynamic_cast<McBopomofoLM*>(lm_.get());
  if (lm == nullptr) {
    return nullptr;
  }
  if (lm->hasAssociatedPhrasesForKey(key)) {
    std::vector<std::string> phrases = lm->associatedPhrasesForKey(key);
    std::vector<InputStates::ChoosingCandidate::Candidate> cs;
    for (const auto& phrase : phrases) {
      cs.emplace_back(phrase, phrase);
    }
    return std::make_unique<InputStates::AssociatedPhrasesPlain>(cs);
  }
  return nullptr;
}

bool KeyHandler::hasDictionaryServices() {
  return dictionaryServices_->hasServices();
}

std::unique_ptr<InputStates::SelectingDictionary>
KeyHandler::buildSelectingDictionaryState(
    std::unique_ptr<InputStates::NotEmpty> nonEmptyState,
    const std::string& selectedPhrase, size_t selectedIndex) {
  std::vector<std::string> menu =
      dictionaryServices_->menuForPhrase(selectedPhrase);
  return std::make_unique<InputStates::SelectingDictionary>(
      std::move(nonEmptyState), selectedPhrase, selectedIndex, menu);
}

size_t KeyHandler::actualCandidateCursorIndex() {
  size_t cursor = grid_.cursor();

  // If the cursor is at the end, always return cursor - 1. Even though
  // ReadingGrid already handles this edge case, we want to use this value
  // consistently. UserOverrideModel also requires the cursor to be this
  // correct value.
  if (cursor == grid_.length() && cursor > 0) {
    return cursor - 1;
  }

  // ReadingGrid already makes the assumption that the cursor is always *at*
  // the reading location, and when selectPhraseAfterCursorAsCandidate_ is true
  // we don't need to do anything. Rather, it's when the flag is false (the
  // default value), that we want to decrement the cursor by one.
  if (!selectPhraseAfterCursorAsCandidate_ && cursor > 0) {
    return cursor - 1;
  }

  return cursor;
}

size_t KeyHandler::candidateCursorIndex() {
  size_t cursor = grid_.cursor();
  return cursor;
}

#pragma endregion Build_States

void KeyHandler::pinNode(
    const InputStates::ChoosingCandidate::Candidate& candidate,
    bool useMoveCursorAfterSelectionSetting) {
  size_t actualCursor = actualCandidateCursorIndex();
  Formosa::Gramambular2::ReadingGrid::Candidate gridCandidate(candidate.reading,
                                                              candidate.value);
  if (!grid_.overrideCandidate(actualCursor, gridCandidate)) {
    return;
  }

  Formosa::Gramambular2::ReadingGrid::WalkResult prevWalk = latestWalk_;
  walk();

  // Update the user override model if warranted.
  size_t accumulatedCursor = 0;
  auto nodeIter = latestWalk_.findNodeAt(actualCursor, &accumulatedCursor);
  if (nodeIter == latestWalk_.nodes.cend()) {
    return;
  }
  const Formosa::Gramambular2::ReadingGrid::NodePtr& currentNode = *nodeIter;

  if (currentNode != nullptr &&
      currentNode->currentUnigram().score() > kNoOverrideThreshold) {
    userOverrideModel_.observe(prevWalk, latestWalk_, actualCursor,
                               GetEpochNowInSeconds());
  }

  if (currentNode != nullptr && useMoveCursorAfterSelectionSetting &&
      moveCursorAfterSelection_) {
    grid_.setCursor(accumulatedCursor);
  }
}

void KeyHandler::pinNode(size_t cursor, const std::string& candidate,
                         const std::string& associatePhrase) {
  if (!grid_.overrideCandidate(cursor, candidate)) {
    return;
  }
  Formosa::Gramambular2::ReadingGrid::WalkResult prevWalk = latestWalk_;
  walk();

  // Update the user override model if warranted.
  size_t accumulatedCursor = 0;
  auto nodeIter = latestWalk_.findNodeAt(cursor, &accumulatedCursor);
  if (nodeIter == latestWalk_.nodes.cend()) {
    return;
  }
  const Formosa::Gramambular2::ReadingGrid::NodePtr& currentNode = *nodeIter;
  if (currentNode != nullptr &&
      currentNode->currentUnigram().score() > kNoOverrideThreshold) {
    userOverrideModel_.observe(prevWalk, latestWalk_, cursor,
                               GetEpochNowInSeconds());
  }
  grid_.setCursor(accumulatedCursor);
  std::vector<std::string> characters = Split(associatePhrase);
  auto* lm = dynamic_cast<McBopomofoLM*>(lm_.get());
  size_t index = 0;
  if (lm != nullptr) {
    for (const auto& character : characters) {
      std::string reading = lm->getReading(character);
      grid_.insertReading(reading);
      grid_.overrideCandidate(accumulatedCursor + index, character);
      index++;
    }
  }
  walk();
}

void KeyHandler::walk() { latestWalk_ = grid_.walk(); }

}  // namespace McBopomofo
