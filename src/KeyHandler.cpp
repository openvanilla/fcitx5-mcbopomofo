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
#include <sstream>
#include <utility>

#include "ChineseNumbers/ChineseNumbers.h"
#include "ChineseNumbers/SuzhouNumbers.h"
#include "Log.h"
#include "UTF8Helper.h"

namespace McBopomofo {

constexpr char kSpaceSeparator[] = " ";
constexpr char kPunctuationListKey = '`';  // Hit the key to bring up the list.
constexpr char kPunctuationListUnigramKey[] = "_punctuation_list";
constexpr char kPunctuationKeyPrefix[] = "_punctuation_";
constexpr char kCtrlPunctuationKeyPrefix[] = "_ctrl_punctuation_";
constexpr char kHalfWidthPunctuationKeyPrefix[] = "_half_punctuation_";
constexpr char kLetterPrefix[] = "_letter_";
constexpr size_t kMinValidMarkingReadingCount = 2;
constexpr size_t kMaxValidMarkingReadingCount = 8;
constexpr size_t kMaxChineseNumberConversionDigits = 20;

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
  if (key.ascii == '\\' && key.ctrlPressed) {
    stateCallback(std::make_unique<InputStates::Empty>());
    stateCallback(std::make_unique<InputStates::SelectingFeature>(
        [this](std::string input) {
          auto* lm = dynamic_cast<McBopomofoLM*>(this->lm_.get());
          if (lm != nullptr) {
            return lm->convertMacro(input);
          }
          return input;
        }));
    reset();
    return true;
  }

  auto* chineseNumber = dynamic_cast<InputStates::ChineseNumber*>(state);
  if (chineseNumber != nullptr) {
    return handleChineseNumber(key, chineseNumber, stateCallback,
                               errorCallback);
  }

  auto* enclosingNumber = dynamic_cast<InputStates::EnclosingNumber*>(state);
  if (enclosingNumber != nullptr) {
    return handleEnclosingNumber(key, enclosingNumber, stateCallback,
                                 errorCallback);
  }

  // From Key's definition, if shiftPressed is true, it can't be a simple key
  // that can be represented by ASCII.
  char simpleAscii =
      (key.ctrlPressed || key.shiftPressed || key.isFromNumberPad) ? '\0'
                                                                   : key.ascii;

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
    if (inputMode_ == McBopomofo::InputMode::McBopomofo &&
        associatedPhrasesEnabled_) {
      auto inputting = buildInputtingState();
      auto copy = std::make_unique<InputStates::Inputting>(*inputting);
      stateCallback(std::move(inputting));
      handleAssociatedPhrases(copy.get(), stateCallback, errorCallback, true);
    } else if (inputMode_ == McBopomofo::InputMode::PlainBopomofo) {
      auto inputting = buildInputtingState();
      auto choosingCandidate =
          buildChoosingCandidateState(inputting.get(), grid_.cursor());
      if (choosingCandidate->candidates.size() == 1) {
        reset();
        std::string reading = choosingCandidate->candidates[0].reading;
        std::string value = choosingCandidate->candidates[0].value;
        auto committingState = std::make_unique<InputStates::Committing>(value);
        stateCallback(std::move(committingState));

        if (associatedPhrasesEnabled_) {
          auto associatedPhrasesPlainState =
              buildAssociatedPhrasesPlainState(reading, value);
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

  // Shift + Space: emit a space directly.
  // Space may also be used to insert space if user has configured it to do so.
  if (key.ascii == Key::SPACE &&
      (key.shiftPressed || !chooseCandidateUsingSpace_)) {
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
    size_t originalCursor = grid_.cursor();
    if (originalCursor == grid_.length() &&
        selectPhraseAfterCursorAsCandidate_ && moveCursorAfterSelection_) {
      grid_.setCursor(originalCursor - 1);
    }
    auto candidateState = buildChoosingCandidateState(
        buildInputtingState().get(), originalCursor);
    stateCallback(std::move(candidateState));
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
    return handleTabKey(key.shiftPressed, state, stateCallback, errorCallback);
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
    if (shiftEnterEnabled_ && key.shiftPressed &&
        inputMode_ == InputMode::McBopomofo) {
      handleAssociatedPhrases(dynamic_cast<InputStates::Inputting*>(state),
                              stateCallback, errorCallback, false);
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
      if (ctrlEnterKey_ == KeyHandlerCtrlEnter::OutputHanyuPinyin) {
        auto output = getHanyuPinyin();
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

      size_t originalCursor = grid_.cursor();
      if (selectPhraseAfterCursorAsCandidate_) {
        grid_.setCursor(originalCursor - 1);
      }
      auto inputtingState = buildInputtingState();
      auto choosingCandidateState =
          buildChoosingCandidateState(inputtingState.get(), originalCursor);
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
      return handlePunctuation(unigram, state, stateCallback, errorCallback);
    }

    if (halfWidthPunctuationEnabled_) {
      unigram = std::string(kHalfWidthPunctuationKeyPrefix) +
                GetKeyboardLayoutName(reading_.keyboardLayout()) + "_" + chrStr;
      if (handlePunctuation(unigram, state, stateCallback, errorCallback)) {
        return true;
      }

      unigram = std::string(kHalfWidthPunctuationKeyPrefix) + chrStr;
      if (handlePunctuation(unigram, state, stateCallback, errorCallback)) {
        return true;
      }
    }

    // Bopomofo layout-specific punctuation handling.
    unigram = std::string(kPunctuationKeyPrefix) +
              GetKeyboardLayoutName(reading_.keyboardLayout()) + "_" + chrStr;
    if (handlePunctuation(unigram, state, stateCallback, errorCallback)) {
      return true;
    }

    // Not handled, try generic punctuations.
    unigram = std::string(kPunctuationKeyPrefix) + chrStr;
    if (handlePunctuation(unigram, state, stateCallback, errorCallback)) {
      return true;
    }

    // Upper case letters.
    if (simpleAscii >= 'A' && simpleAscii <= 'Z') {
      if (putLowercaseLettersToComposingBuffer_) {
        unigram = std::string(kLetterPrefix) + chrStr;

        // Ignore return value, since we always return true below.
        handlePunctuation(unigram, state, stateCallback, errorCallback);
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
    size_t originalCursor, StateCallback stateCallback) {
  if (inputMode_ == InputMode::PlainBopomofo) {
    reset();
    std::string reading = candidate.reading;
    std::string value = candidate.value;
    std::unique_ptr<InputStates::Committing> committingState =
        std::make_unique<InputStates::Committing>(value);
    stateCallback(std::move(committingState));

    if (associatedPhrasesEnabled_) {
      auto associatedPhrasesPlainState =
          buildAssociatedPhrasesPlainState(reading, value);
      if (associatedPhrasesPlainState != nullptr) {
        stateCallback(std::move(associatedPhrasesPlainState));
      }
    }
    return;
  }

  pinNode(originalCursor, candidate);
  auto inputting = buildInputtingState();
  auto copy = std::make_unique<InputStates::Inputting>(*inputting);
  stateCallback(std::move(inputting));

  if (associatedPhrasesEnabled_) {
    handleAssociatedPhrases(
        dynamic_cast<InputStates::Inputting*>(copy.get()), stateCallback,
        []() {}, true);
  }
}

void KeyHandler::candidateAssociatedPhraseSelected(
    size_t cursorIndex,
    const InputStates::ChoosingCandidate::Candidate& candidate,
    const std::string& selectedReading, const std::string& selectedValue,
    const StateCallback& stateCallback) {
  pinNodeWithAssociatedPhrase(cursorIndex, selectedReading, selectedValue,
                              candidate.reading, candidate.value);
  stateCallback(buildInputtingState());
}

void KeyHandler::dictionaryServiceSelected(std::string phrase, size_t index,
                                           InputState* currentState,
                                           StateCallback stateCallback) {
  dictionaryServices_->lookup(std::move(phrase), index, currentState,
                              stateCallback);
}

void KeyHandler::candidatePanelCancelled(size_t originalCursor,
                                         StateCallback stateCallback) {
  if (inputMode_ == InputMode::PlainBopomofo) {
    reset();
    std::unique_ptr<InputStates::EmptyIgnoringPrevious>
        emptyIgnorePreviousState =
            std::make_unique<InputStates::EmptyIgnoringPrevious>();
    stateCallback(std::move(emptyIgnorePreviousState));
    return;
  }
  grid_.setCursor(originalCursor);
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

void KeyHandler::boostPhrase(const std::string& reading,
                             const std::string& value) {
  userPhraseAdder_->addUserPhrase(reading, value);
  onAddNewPhrase_(value);
}

void KeyHandler::excludePhrase(const std::string& reading,
                               const std::string& value) {
  userPhraseAdder_->removeUserPhrase(reading, value);
  onAddNewPhrase_(value);
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

void KeyHandler::setShiftEnterEnabled(bool flag) { shiftEnterEnabled_ = flag; }

void KeyHandler::setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior) {
  ctrlEnterKey_ = behavior;
}

void KeyHandler::setAssociatedPhrasesEnabled(bool enabled) {
  associatedPhrasesEnabled_ = enabled;
}

void KeyHandler::setHalfWidthPunctuationEnabled(bool enabled) {
  halfWidthPunctuationEnabled_ = enabled;
}

void KeyHandler::setOnAddNewPhrase(
    std::function<void(const std::string&)> onAddNewPhrase) {
  onAddNewPhrase_ = std::move(onAddNewPhrase);
}

void KeyHandler::setRepeatedPunctuationToSelectCandidateEnabled(bool enabled) {
  repeatedPunctuationToSelectCandidateEnabled_ = enabled;
}

void KeyHandler::setChooseCandidateUsingSpace(bool enabled) {
  chooseCandidateUsingSpace_ = enabled;
}

#pragma endregion Settings

#pragma region Key_Handling

bool KeyHandler::handleAssociatedPhrases(InputStates::Inputting* state,
                                         StateCallback stateCallback,
                                         ErrorCallback errorCallback,
                                         bool useShiftKey) {
  size_t cursor = grid_.cursor();

  // We need to find the node *before* the cursor, so cursor must be >= 1.
  if (cursor < 1) {
    errorCallback();
    return true;
  }
  auto* inputting = dynamic_cast<InputStates::Inputting*>(state);
  if (inputting != nullptr) {
    // Find the selected node *before* the cursor.
    size_t prefixCursorIndex = cursor - 1;

    size_t endCursorIndex = 0;
    auto nodePtrIt = latestWalk_.findNodeAt(prefixCursorIndex, &endCursorIndex);
    if (nodePtrIt == latestWalk_.nodes.cend() || endCursorIndex == 0) {
      // Shouldn't happen.
      errorCallback();
      return true;
    }

    // Validate that the value's codepoint count is the same as the number
    // of readings. This is a strict requirement for the associated phrases.
    std::vector<std::string> codepoints = Split((*nodePtrIt)->value());
    std::vector<std::string> readings =
        AssociatedPhrasesV2::SplitReadings((*nodePtrIt)->reading());
    if (codepoints.size() != readings.size()) {
      errorCallback();
      return true;
    }

    if (endCursorIndex < readings.size()) {
      // Shouldn't happen.
      errorCallback();
      return true;
    }

    // Try to find the longest associated phrase prefix. Suppose we have
    // a walk A-B-CD-EFGH and the cursor is between EFG and H. Our job is
    // to try the prefixes EFG, EF, and G to see which one yields a list
    // of possible associated phrases.
    //
    //             grid_->cursor()
    //                 |
    //                 v
    //     A-B-C-D-|EFG|H|
    //             ^     ^
    //             |     |
    //             |    endCursorIndex
    //           startCursorIndex
    //
    // In this case, the max prefix length is 3. This works because our
    // association phrases mechanism require that the node's codepoint
    // length and reading length (i.e. the spanning length) must be equal.
    //
    // And say if the prefix "FG" has associated phrases FGPQ, FGRST, and
    // the user later chooses FGRST, we will first override the FG node
    // again, essentially breaking that from E and H (the vertical bar
    // represents the cursor):
    //
    //     A-B-C-D-E'-FG|-H'
    //
    // And then we add the readings for the RST to the grid, and override
    // the grid at the cursor position with the value FGRST (and its
    // corresponding reading) again, so that the process is complete:
    //
    //     A-B-C-D-E'-FGRST|-H'
    //
    // Notice that after breaking FG from EFGH, the values E and H may
    // change due to a new walk, hence the notation E' and H'. We address
    // issue in pinNodeWithAssociatedPhrase() by making sure that the nodes
    // will be overridden with the values E and H.
    size_t startCursorIndex = endCursorIndex - readings.size();
    size_t prefixLength = cursor - startCursorIndex;
    size_t maxPrefixLength = prefixLength;
    for (; prefixLength > 0; --prefixLength) {
      size_t startIndex = maxPrefixLength - prefixLength;
      auto cpBegin = codepoints.cbegin();
      auto cpEnd = codepoints.cbegin();
      std::advance(cpBegin, startIndex);
      std::advance(cpEnd, maxPrefixLength);
      auto cpSlice = std::vector<std::string>(cpBegin, cpEnd);

      auto rdBegin = readings.cbegin();
      auto rdEnd = readings.cbegin();
      std::advance(rdBegin, startIndex);
      std::advance(rdEnd, maxPrefixLength);
      auto rdSlice = std::vector<std::string>(rdBegin, rdEnd);

      std::stringstream value;
      for (const std::string& cp : cpSlice) {
        value << cp;
      }

      auto associatedPhrasesState = buildAssociatedPhrasesState(
          buildInputtingState(), prefixCursorIndex,
          AssociatedPhrasesV2::CombineReadings(rdSlice), value.str(),
          /*selectedCandidateIndex=*/0, useShiftKey);
      if (associatedPhrasesState != nullptr) {
        stateCallback(std::move(associatedPhrasesState));
        return true;
      }
    }
    errorCallback();
  }

  if (!useShiftKey) {
    errorCallback();
  }

  return true;
}

bool KeyHandler::handleTabKey(bool isShiftPressed,
                              McBopomofo::InputState* state,
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

  const auto candidates =
      buildChoosingCandidateState(inputting, grid_.cursor())->candidates;
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
      if (isShiftPressed) {
        currentIndex = candidates.size() - 1;
      } else {
        currentIndex = 1;
      }
    }
  } else {
    for (const auto& candidate : candidates) {
      if (candidate.reading == currentNode->reading() &&
          candidate.value == currentNode->value()) {
        if (isShiftPressed) {
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

  pinNode(grid_.cursor(), candidates[currentIndex],
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
                                   McBopomofo::InputState* state,
                                   const StateCallback& stateCallback,
                                   const ErrorCallback& errorCallback) {
  if (!lm_->hasUnigrams(punctuationUnigramKey)) {
    return false;
  }

  if (repeatedPunctuationToSelectCandidateEnabled_) {
    size_t prefixCursorIndex = grid_.cursor();
    size_t actualPrefixCursorIndex =
        prefixCursorIndex > 0 ? prefixCursorIndex - 1 : 0;
    auto nodeIter = latestWalk_.findNodeAt(actualPrefixCursorIndex);
    if (nodeIter != latestWalk_.nodes.cend()) {
      const Formosa::Gramambular2::ReadingGrid::NodePtr& currentNode =
          *nodeIter;
      if (currentNode->reading() == punctuationUnigramKey) {
        auto candidates = grid_.candidatesAt(actualPrefixCursorIndex);
        if (candidates.size() > 1) {
          if (selectPhraseAfterCursorAsCandidate_) {
            grid_.setCursor(actualPrefixCursorIndex);
          }
          handleTabKey(false, state, stateCallback, errorCallback);
          grid_.setCursor(prefixCursorIndex);
          stateCallback(buildInputtingState());
          return true;
        }
      }
    }
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
    auto choosingCandidate =
        buildChoosingCandidateState(inputting.get(), grid_.cursor());
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
    auto copy = std::make_unique<InputStates::Inputting>(*inputting);
    stateCallback(std::move(inputting));
    if (associatedPhrasesEnabled_) {
      handleAssociatedPhrases(copy.get(), stateCallback, errorCallback, true);
    }
  }

  return true;
}

bool KeyHandler::handleChineseNumber(
    Key key, McBopomofo::InputStates::ChineseNumber* state,
    StateCallback stateCallback, KeyHandler::ErrorCallback errorCallback) {
  if (key.ascii == Key::ESC) {
    stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
    return true;
  }
  if (key.isDeleteKeys()) {
    std::string number = state->number;
    if (!number.empty()) {
      number = number.substr(0, number.length() - 1);
    } else {
      errorCallback();
      return true;
    }
    auto newState =
        std::make_unique<InputStates::ChineseNumber>(number, state->style);
    stateCallback(std::move(newState));
    return true;
  }
  if (key.ascii == Key::RETURN) {
    if (state->number.empty()) {
      stateCallback(std::make_unique<InputStates::Empty>());
      return true;
    }
    bool commonFound = false;
    std::stringstream intStream;
    std::stringstream decStream;

    for (char c : state->number) {
      if (c == '.') {
        commonFound = true;
        continue;
      }
      if (commonFound) {
        decStream << c;
      } else {
        intStream << c;
      }
    }
    std::string intPart = intStream.str();
    std::string decPart = decStream.str();
    std::string commitSting;
    switch (state->style) {
      case ChineseNumberStyle::LOWER:
        commitSting = ChineseNumbers::Generate(
            intPart, decPart, ChineseNumbers::ChineseNumberCase::LOWERCASE);
        break;
      case ChineseNumberStyle::UPPER:
        commitSting = ChineseNumbers::Generate(
            intPart, decPart, ChineseNumbers::ChineseNumberCase::UPPERCASE);
        break;
      case ChineseNumberStyle::SUZHOU:
        commitSting = SuzhouNumbers::Generate(intPart, decPart, "單位", true);
        break;
      default:
        break;
    }
    auto newState = std::make_unique<InputStates::Committing>(commitSting);
    stateCallback(std::move(newState));
    return true;
  }
  if (key.ascii >= '0' && key.ascii <= '9') {
    if (state->number.length() > kMaxChineseNumberConversionDigits) {
      errorCallback();
      return true;
    }
    std::string newNumber = state->number + key.ascii;
    auto newState =
        std::make_unique<InputStates::ChineseNumber>(newNumber, state->style);
    stateCallback(std::move(newState));
  } else if (key.ascii == '.') {
    if (state->number.find('.') != std::string::npos) {
      errorCallback();
      return true;
    }
    if (state->number.empty() ||
        state->number.length() > kMaxChineseNumberConversionDigits) {
      errorCallback();
      return true;
    }
    std::string newNumber = state->number + key.ascii;
    auto newState =
        std::make_unique<InputStates::ChineseNumber>(newNumber, state->style);
    stateCallback(std::move(newState));

  } else {
    errorCallback();
  }

  return true;
}

bool KeyHandler::handleEnclosingNumber(
    Key key, McBopomofo::InputStates::EnclosingNumber* state,
    StateCallback stateCallback, KeyHandler::ErrorCallback errorCallback) {
  if (key.ascii == Key::ESC) {
    stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
    return true;
  }
  if (key.isDeleteKeys()) {
    std::string number = state->number;
    if (!number.empty()) {
      number = number.substr(0, number.length() - 1);
    } else {
      errorCallback();
      return true;
    }
    auto newState = std::make_unique<InputStates::EnclosingNumber>(number);
    stateCallback(std::move(newState));
    return true;
  }
  if (key.ascii == Key::RETURN || key.ascii == Key::SPACE) {
    if (state->number.empty()) {
      stateCallback(std::make_unique<InputStates::Empty>());
      return true;
    }
    std::string unigramKey = "_number_" + state->number;
    if (!lm_->hasUnigrams(unigramKey)) {
      errorCallback();
      return true;
    }
    auto unigrams = lm_->getUnigrams(unigramKey);
    if (unigrams.size() == 1) {
      auto firstUnigram = unigrams[0];
      std::string value = firstUnigram.value();
      stateCallback(std::make_unique<InputStates::Committing>(value));
      stateCallback(std::make_unique<InputStates::Empty>());
      return true;
    }

    grid_.insertReading(unigramKey);
    walk();
    size_t originalCursor = grid_.cursor();
    if (selectPhraseAfterCursorAsCandidate_) {
      grid_.setCursor(originalCursor - 1);
    }
    auto inputtingState = buildInputtingState();
    auto choosingCandidateState =
        buildChoosingCandidateState(inputtingState.get(), originalCursor);
    stateCallback(std::move(inputtingState));
    stateCallback(std::move(choosingCandidateState));
    return true;
  }
  if (key.ascii >= '0' && key.ascii <= '9') {
    if (state->number.length() > 2) {
      errorCallback();
      return true;
    }
    std::string newNumber = state->number + key.ascii;
    auto newState = std::make_unique<InputStates::EnclosingNumber>(newNumber);
    stateCallback(std::move(newState));
  } else {
    errorCallback();
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

std::string KeyHandler::getHanyuPinyin() {
  std::string composed;
  for (const auto& node : latestWalk_.nodes) {
    std::string key = node->reading();
    std::string value = node->value();

    // If a key starts with underscore, it is usually for a punctuation or a
    // symbol but not a Bopomofo reading, so we just ignore such case.
    if (key.rfind(std::string("_"), 0) == 0) {
      composed += value;
    } else {
      size_t start = 0, end;
      std::string delimiter = "-";
      while ((end = key.find(delimiter, start)) != std::string::npos) {
        auto component = key.substr(start, end - start);
        Formosa::Mandarin::BopomofoSyllable syllable =
            Formosa::Mandarin::BopomofoSyllable::FromComposedString(component);
        std::string hanyuPinyin = syllable.HanyuPinyinString(false, false);
        composed += hanyuPinyin;
        start = end + 1;
      }
      auto component = key.substr(start);
      Formosa::Mandarin::BopomofoSyllable syllable =
          Formosa::Mandarin::BopomofoSyllable::FromComposedString(component);
      std::string hanyuPinyin = syllable.HanyuPinyinString(false, false);
      composed += hanyuPinyin;
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
KeyHandler::buildChoosingCandidateState(InputStates::NotEmpty* nonEmptyState,
                                        size_t originalCursor) {
  auto candidates = grid_.candidatesAt(actualCandidateCursorIndex());
  std::vector<InputStates::ChoosingCandidate::Candidate> stateCandidates;
  for (const auto& c : candidates) {
    stateCandidates.emplace_back(c.reading, c.value);
  }

  return std::make_unique<InputStates::ChoosingCandidate>(
      nonEmptyState->composingBuffer, nonEmptyState->cursorIndex,
      originalCursor, std::move(stateCandidates));
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
    size_t prefixCursorIndex, std::string prefixCombinedReading,
    std::string prefixValue, size_t selectedCandidateIndex, bool useShiftKey) {
  McBopomofoLM* lm = dynamic_cast<McBopomofoLM*>(lm_.get());
  if (lm == nullptr) {
    return nullptr;
  }

  std::vector<std::string> splitReadings =
      AssociatedPhrasesV2::SplitReadings(prefixCombinedReading);
  std::vector<AssociatedPhrasesV2::Phrase> phrases =
      lm->findAssociatedPhrasesV2(prefixValue, splitReadings);

  if (!phrases.empty()) {
    std::vector<InputStates::ChoosingCandidate::Candidate> cs;
    for (const auto& phrase : phrases) {
      // The candidates should contain the prefix.
      cs.emplace_back(phrase.combinedReading(), phrase.value);
    }

    return std::make_unique<InputStates::AssociatedPhrases>(
        std::move(previousState), prefixCursorIndex, prefixCombinedReading,
        prefixValue, selectedCandidateIndex, cs, useShiftKey);
  }
  return nullptr;
}

std::unique_ptr<InputStates::AssociatedPhrases>
KeyHandler::buildAssociatedPhrasesStateFromCandidateChoosingState(
    std::unique_ptr<InputStates::NotEmpty> previousState,
    size_t candidateCursorIndex, std::string prefixCombinedReading,
    std::string prefixValue, size_t selectedCandidateIndex) {
  return buildAssociatedPhrasesState(
      std::move(previousState),
      computeActualCandidateCursorIndex(candidateCursorIndex),
      prefixCombinedReading, prefixValue, selectedCandidateIndex, false);
}

std::unique_ptr<InputStates::AssociatedPhrasesPlain>
KeyHandler::buildAssociatedPhrasesPlainState(const std::string& reading,
                                             const std::string& value) {
  McBopomofoLM* lm = dynamic_cast<McBopomofoLM*>(lm_.get());
  if (lm == nullptr) {
    return nullptr;
  }

  std::vector<std::string> splitReadings =
      AssociatedPhrasesV2::SplitReadings(reading);
  std::vector<McBopomofo::AssociatedPhrasesV2::Phrase> phrases =
      lm->findAssociatedPhrasesV2(value, splitReadings);
  if (phrases.empty()) {
    return nullptr;
  }

  std::vector<InputStates::ChoosingCandidate::Candidate> cs;
  for (const auto& phrase : phrases) {
    // Chop the prefix of AssociatedPhrasesV2::Phrase's readings.
    auto pri = phrase.readings.cbegin();
    auto pre = phrase.readings.cend();
    for (auto sri = splitReadings.cbegin(), sre = splitReadings.cend();
         sri != sre; ++sri) {
      if (pri != pre) {
        ++pri;
      }
    }
    if (pri == pre) {
      // Shouldn't happen.
      continue;
    }
    std::string combinedReadingWithoutPrefix =
        AssociatedPhrasesV2::CombineReadings(
            std::vector<std::string>(pri, pre));

    // Ditto for the value.
    std::string valueWithoutPrefix = phrase.value.substr(value.length());

    cs.emplace_back(combinedReadingWithoutPrefix, valueWithoutPrefix);
  }

  if (!cs.empty()) {
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
  return computeActualCandidateCursorIndex(grid_.cursor());
}

size_t KeyHandler::computeActualCandidateCursorIndex(size_t index) {
  if (index > grid_.length()) {
    return grid_.length() > 0 ? grid_.length() - 1 : 0;
  }

  // If the index is at the end, always return index - 1. Even though
  // ReadingGrid already handles this edge case, we want to use this value
  // consistently. UserOverrideModel also requires the index to be this
  // correct value.
  if (index == grid_.length() && index > 0) {
    return index - 1;
  }

  // ReadingGrid already makes the assumption that the index is always *at*
  // the reading location, and when selectPhraseAfterCursorAsCandidate_ is true
  // we don't need to do anything. Rather, it's when the flag is false (the
  // default value), that we want to decrement the index by one.
  if (!selectPhraseAfterCursorAsCandidate_ && index > 0) {
    return index - 1;
  }

  return index;
}

size_t KeyHandler::candidateCursorIndex() {
  size_t cursor = grid_.cursor();
  return cursor;
}

void KeyHandler::setCandidateCursorIndex(size_t newCursor) {
  if (newCursor > grid_.length()) {
    newCursor = grid_.length();
  }
  grid_.setCursor(newCursor);
}

#pragma endregion Build_States

void KeyHandler::pinNode(
    size_t originalCursor,
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
  } else {
    grid_.setCursor(originalCursor);
  }
}

void KeyHandler::pinNodeWithAssociatedPhrase(
    size_t prefixCursorIndex, const std::string& prefixReading,
    const std::string& prefixValue, const std::string& associatedPhraseReading,
    const std::string& associatedPhraseValue) {
  if (grid_.length() == 0) {
    return;
  }

  // Unlike actualCandidateCursorIndex() which takes the Hanyin/MS IME cursor
  // modes into consideration, prefixCursorIndex is *already* the actual node
  // position in the grid. The only boundary condition is when prefixCursorIndex
  // is at the end. That's when we should decrement by one.
  size_t actualPrefixCursorIndex = (prefixCursorIndex == grid_.length())
                                       ? prefixCursorIndex - 1
                                       : prefixCursorIndex;
  // First of all, let's find the target node where the prefix is found. The
  // node may not be exactly the same as the prefix.
  size_t accumulatedCursor = 0;
  auto nodeIter =
      latestWalk_.findNodeAt(actualPrefixCursorIndex, &accumulatedCursor);

  // Should not happen. The end location must be >= the node's spanning length.
  if (accumulatedCursor < (*nodeIter)->spanningLength()) {
    return;
  }

  // Let's do a split override. If a node is now ABCD, let's make four overrides
  // A-B-C-D, essentially splitting the node. Why? Because we're inserting an
  // associated phrase. Say the phrase is BCEF with the prefix BC. If we don't
  // do the override, the nodes that represent A and D may not carry the same
  // values after the next walk, since the underlying reading is now a-bcef-d
  // and that does not necessary guarantee that A and D will be there.
  std::vector<std::string> originalNodeValues = Split((*nodeIter)->value());
  if (originalNodeValues.size() == (*nodeIter)->spanningLength()) {
    // Only performs this if the condition is satisfied.
    size_t overrideIndex = accumulatedCursor - (*nodeIter)->spanningLength();
    for (const auto& value : originalNodeValues) {
      grid_.overrideCandidate(overrideIndex, value);
      ++overrideIndex;
    }
  }

  // Now, we override the prefix candidate again. This provides us with
  // information for how many more we need to fill in to complete the
  // associated phrase.
  Formosa::Gramambular2::ReadingGrid::Candidate prefixCandidate{prefixReading,
                                                                prefixValue};
  if (!grid_.overrideCandidate(actualPrefixCursorIndex, prefixCandidate)) {
    return;
  }
  walk();

  // Now we've set ourselves up. Because associated phrases require the strict
  // one-reading-for-one-value rule, we can comfortably count how many readings
  // we'll need to insert. First, let's move to the end of the newly overridden
  // phrase.
  nodeIter =
      latestWalk_.findNodeAt(actualPrefixCursorIndex, &accumulatedCursor);
  grid_.setCursor(accumulatedCursor);

  // Compute how many more reading do we have to insert.
  std::vector<std::string> associatedPhraseValues =
      McBopomofo::Split(associatedPhraseValue);

  size_t nodeSpanningLength = (*nodeIter)->spanningLength();
  std::vector<std::string> splitReadings =
      AssociatedPhrasesV2::SplitReadings(associatedPhraseReading);
  size_t splitReadingsSize = splitReadings.size();
  if (nodeSpanningLength >= splitReadingsSize) {
    // Shouldn't happen
    return;
  }

  for (size_t i = nodeSpanningLength; i < splitReadingsSize; i++) {
    grid_.insertReading(splitReadings[i]);
    ++accumulatedCursor;
    // For each node, we assign the value of the corresponding phrase.
    // If the phrase is not found in the phrase database, we perform a fallback
    // instead.
    if (i < associatedPhraseValues.size()) {
      grid_.overrideCandidate(accumulatedCursor, associatedPhraseValues[i]);
    }
    grid_.setCursor(accumulatedCursor);
  }

  // Finally, let's override with the full associated phrase's value.
  if (!grid_.overrideCandidate(actualPrefixCursorIndex,
                               associatedPhraseValue)) {
    // Shouldn't happen
  }

  walk();
  // Cursor is already at accumulatedCursor, so no more work here.
}

void KeyHandler::walk() { latestWalk_ = grid_.walk(); }

}  // namespace McBopomofo
