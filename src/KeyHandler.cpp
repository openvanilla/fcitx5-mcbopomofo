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
#include <utility>

#include "UTF8Helper.h"

namespace McBopomofo {

static constexpr const char* const kJoinSeparator = "-";
static constexpr const char* const kPunctationListKey = "_punctuation_list";
static constexpr const char* const kPunctationKeyPrefix = "_punctuation_";

// Maximum composing buffer size, roughly in codepoints.
// TODO(unassigned): maybe make this configurable.
constexpr size_t kComposingBufferSize = 10;

static const char* GetKeyboardLayoutName(
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
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
}

KeyHandler::KeyHandler(
    std::shared_ptr<Formosa::Gramambular::LanguageModel> languageModel)
    : reading_(Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout()) {
  languageModel_ = std::move(languageModel);
  builder_ = std::make_unique<Formosa::Gramambular::BlockReadingBuilder>(
      languageModel_.get());
  builder_->setJoinSeparator(kJoinSeparator);
}

bool KeyHandler::handle(fcitx::Key key, McBopomofo::InputState* state,
                        StateCallback stateCallback,
                        ErrorCallback errorCallback) {
  // key.isSimple() is true => key.sym() guaranteed to be printable ASCII.
  char asciiChar = key.isSimple() ? key.sym() : 0;

  // See if it's valid BPMF reading.
  if (reading_.isValidKey(asciiChar)) {
    reading_.combineKey(asciiChar);

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
      reading_.hasToneMarker() ||
      (!reading_.isEmpty() && key.check(FcitxKey_space));

  if (shouldComposeReading) {
    std::string syllable = reading_.syllable().composedString();
    reading_.clear();

    if (!languageModel_->hasUnigramsForKey(syllable)) {
      errorCallback();
      stateCallback(buildInputtingState());
      return true;
    }

    builder_->insertReadingAtCursor(syllable);
    std::string evictedText = popEvictedTextAndWalk();

    auto inputtingState = buildInputtingState();
    inputtingState->setPoppedText(evictedText);
    stateCallback(std::move(inputtingState));
    return true;
  }

  // Space hit: see if we should enter the candidate choosing state.
  InputStates::NotEmpty* maybeNotEmptyState =
      dynamic_cast<InputStates::NotEmpty*>(state);
  if (key.check(FcitxKey_space) && maybeNotEmptyState != nullptr &&
      reading_.isEmpty()) {
    stateCallback(bulidChoosingCandidateState(maybeNotEmptyState));
    return true;
  }

  // Esc hit.
  if (key.check(FcitxKey_Escape)) {
    if (maybeNotEmptyState == nullptr) {
      return false;
    }

    if (!reading_.isEmpty()) {
      reading_.clear();
      if (!builder_->length()) {
        stateCallback(std::make_unique<InputStates::Empty>());
      } else {
        auto inputtingState = buildInputtingState();
        stateCallback(std::move(inputtingState));
      }
    }
    return true;
  }

  // Cursor keys.
  static std::array<fcitx::Key, 4> cursorKeys{
      fcitx::Key(FcitxKey_Left), fcitx::Key(FcitxKey_Right),
      fcitx::Key(FcitxKey_Home), fcitx::Key(FcitxKey_End)};
  if (key.checkKeyList(cursorKeys)) {
    return handleCursorKeys(key, state, stateCallback, errorCallback);
  }

  // Backspace and Del.
  static std::array<fcitx::Key, 2> deleteKeys{fcitx::Key(FcitxKey_BackSpace),
                                              fcitx::Key(FcitxKey_Delete)};
  if (key.checkKeyList(deleteKeys)) {
    return handleDeleteKeys(key, state, stateCallback, errorCallback);
  }

  // Enter.
  if (key.check(FcitxKey_Return)) {
    if (maybeNotEmptyState == nullptr) {
      return false;
    }

    if (!reading_.isEmpty()) {
      errorCallback();
      stateCallback(buildInputtingState());
      return true;
    }

    auto inputtingState = buildInputtingState();
    auto committingState = std::make_unique<InputStates::Committing>();
    // Steal the composed text built by the inputting state.
    committingState->setPoppedText(inputtingState->composingBuffer());
    stateCallback(std::move(committingState));
    reset();
    return true;
  }

  // Punctuation key: backtick or grave accent.
  if (key.check(FcitxKey_grave) &&
      languageModel_->hasUnigramsForKey(kPunctationListKey)) {
    if (reading_.isEmpty()) {
      builder_->insertReadingAtCursor(kPunctationListKey);

      std::string evictedText = popEvictedTextAndWalk();

      auto inputtingState = buildInputtingState();
      inputtingState->setPoppedText(evictedText);
      auto choosingCanidateState =
          bulidChoosingCandidateState(inputtingState.get());
      stateCallback(std::move(inputtingState));
      stateCallback(std::move(choosingCanidateState));
    } else {
      // Punctuation ignored if a bopomofo reading is active..
      errorCallback();
    }
    return true;
  }

  if (asciiChar != 0) {
    std::string chrStr(1, asciiChar);

    // Bopomofo layout-specific punctuation handling.
    std::string unigram = std::string(kPunctationKeyPrefix) +
                          GetKeyboardLayoutName(reading_.keyboardLayout()) +
                          "_" + chrStr;
    if (handlePunctuation(unigram, stateCallback, errorCallback)) {
      return true;
    }

    // Not handled, try generic punctuations.
    unigram = std::string(kPunctationKeyPrefix) + chrStr;
    if (handlePunctuation(unigram, stateCallback, errorCallback)) {
      return true;
    }
  }

  // No key is handled. Refresh and consume the key.
  if (maybeNotEmptyState != nullptr) {
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  return false;
}

void KeyHandler::candidateSelected(const std::string& candidate,
                                   KeyHandler::StateCallback stateCallback) {
  pinNode(candidate);
  stateCallback(buildInputtingState());
}

void KeyHandler::candidatePanelCancelled(
    KeyHandler::StateCallback stateCallback) {
  stateCallback(buildInputtingState());
}

void KeyHandler::reset() {
  reading_.clear();
  builder_->clear();
  walkedNodes_.clear();
}

void KeyHandler::setKeyboardLayout(
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
  reading_.setKeyboardLayout(layout);
}

void KeyHandler::setSelectPhraseAfterCursorAsCandidate(bool flag) {
  selectPhraseAfterCursorAsCandidate_ = flag;
}

bool KeyHandler::handleCursorKeys(fcitx::Key key, McBopomofo::InputState* state,
                                  KeyHandler::StateCallback stateCallback,
                                  KeyHandler::ErrorCallback errorCallback) {
  if (dynamic_cast<InputStates::Inputting*>(state) == nullptr) {
    return false;
  }

  if (!reading_.isEmpty()) {
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  bool isValidMove = false;
  switch (key.sym()) {
    case FcitxKey_Left:
      if (builder_->cursorIndex() > 0) {
        builder_->setCursorIndex(builder_->cursorIndex() - 1);
        isValidMove = true;
      }
      break;
    case FcitxKey_Right:
      if (builder_->cursorIndex() < builder_->length()) {
        builder_->setCursorIndex(builder_->cursorIndex() + 1);
        isValidMove = true;
      }
      break;
    case FcitxKey_Home:
      builder_->setCursorIndex(0);
      isValidMove = true;
      break;
    case FcitxKey_End:
      builder_->setCursorIndex(builder_->length());
      isValidMove = true;
      break;
    default:
      // Ignored.
      break;
  }

  if (isValidMove) {
    stateCallback(buildInputtingState());
  } else {
    errorCallback();
    stateCallback(buildInputtingState());
  }
  return true;
}

bool KeyHandler::handleDeleteKeys(fcitx::Key key, McBopomofo::InputState* state,
                                  KeyHandler::StateCallback stateCallback,
                                  KeyHandler::ErrorCallback errorCallback) {
  if (dynamic_cast<InputStates::Inputting*>(state) == nullptr) {
    return false;
  }

  if (reading_.isEmpty()) {
    bool isValidDelete = false;

    if (key.check(FcitxKey_BackSpace) && builder_->cursorIndex() > 0) {
      builder_->deleteReadingBeforeCursor();
      isValidDelete = true;
    } else if (key.check(FcitxKey_Delete) &&
               builder_->cursorIndex() < builder_->length()) {
      builder_->deleteReadingAfterCursor();
      isValidDelete = true;
    }
    if (!isValidDelete) {
      errorCallback();
      stateCallback(buildInputtingState());
      return true;
    }
    walk();
  } else {
    if (key.check(FcitxKey_BackSpace)) {
      reading_.backspace();
    } else {
      // Del not supported when bopomofo reading is active.
      errorCallback();
    }
  }

  if (reading_.isEmpty() && builder_->length() == 0) {
    // Cancel the previous input state if everything is empty now.
    stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
  } else {
    stateCallback(buildInputtingState());
  }
  return true;
}

bool KeyHandler::handlePunctuation(std::string punctuationUnigramKey,
                                   KeyHandler::StateCallback stateCallback,
                                   KeyHandler::ErrorCallback errorCallback) {
  if (!languageModel_->hasUnigramsForKey(punctuationUnigramKey)) {
    return false;
  }

  if (!reading_.isEmpty()) {
    errorCallback();
    stateCallback(buildInputtingState());
    return true;
  }

  builder_->insertReadingAtCursor(punctuationUnigramKey);
  std::string evictedText = popEvictedTextAndWalk();

  auto inputtingState = buildInputtingState();
  inputtingState->setPoppedText(evictedText);
  stateCallback(std::move(inputtingState));
  return true;
}

std::unique_ptr<InputStates::Inputting> KeyHandler::buildInputtingState() {
  // To construct an Inputting state, we need to first retrieve the entire
  // composing buffer from the current grid, then split the composed string into
  // head and tail, so that we can insert the current reading (if not-empty)
  // between them.
  //
  // We'll also need to compute the UTF-8 cursor index. The idea here is we use
  // a "running" index that will eventually catch the cursor index in the
  // builder. The tricky part is that if the spanning length of the node that
  // the cursor is at does not agree with the actual codepoint count of the
  // node's value, we'll need to move the cursor at the end of the node to avoid
  // confusions.

  size_t runningCursor = 0;  // spanning-length-based, like the builder cursor
  size_t builderCursor = builder_->cursorIndex();

  std::string composed;
  size_t composedCursor =
      0;  // UTF-8 (so "byte") cursor per fcitx5 requirement.

  for (const Formosa::Gramambular::NodeAnchor& anchor : walkedNodes_) {
    const Formosa::Gramambular::Node* node = anchor.node;
    if (node == nullptr) {
      continue;
    }

    const std::string& value = node->currentKeyValue().value;
    composed += value;

    // No work if runningCursor has already caught up with builderCursor.
    if (runningCursor == builderCursor) {
      continue;
    }
    size_t spanningLength = anchor.spanningLength;

    // Simple case: if the running cursor is behind, add the spanning length.
    if (runningCursor + spanningLength <= builderCursor) {
      composedCursor += value.length();
      runningCursor += spanningLength;
      continue;
    }

    // The builder cursor is in the middle of the node.
    size_t distance = builderCursor - runningCursor;
    std::u32string u32Value = ToU32(value);

    // The actual partial value's code point length is the shorter of the
    // distance and the value's code point count.
    size_t cpLen = std::min(distance, u32Value.length());
    std::u32string actualU32Value(u32Value.begin(), u32Value.begin() + cpLen);
    std::string actualValue = ToU8(actualU32Value);
    composedCursor += actualValue.length();
    runningCursor += distance;
  }

  std::string head = composed.substr(0, composedCursor);
  std::string reading = reading_.composedString();
  std::string tail =
      composed.substr(composedCursor, composed.length() - composedCursor);

  auto state = std::make_unique<InputStates::Inputting>();
  state->setComposingBuffer(head + reading + tail);
  state->setCursorIndex(composedCursor + reading.length());
  return state;
}

std::unique_ptr<InputStates::ChoosingCandidate>
KeyHandler::bulidChoosingCandidateState(InputStates::NotEmpty* nonEmptyState) {
  std::vector<Formosa::Gramambular::NodeAnchor> anchoredNodes =
      builder_->grid().nodesCrossingOrEndingAt(actualCandidateCursorIndex());

  // sort the nodes, so that longer nodes (representing longer phrases) are
  // placed at the top of the candidate list
  stable_sort(anchoredNodes.begin(), anchoredNodes.end(),
              [](const Formosa::Gramambular::NodeAnchor& a,
                 const Formosa::Gramambular::NodeAnchor& b) {
                return a.node->key().length() > b.node->key().length();
              });

  std::vector<std::string> candidates;
  for (const Formosa::Gramambular::NodeAnchor& anchor : anchoredNodes) {
    const std::vector<Formosa::Gramambular::KeyValuePair>& nodeCandidates =
        anchor.node->candidates();
    for (const Formosa::Gramambular::KeyValuePair& kv : nodeCandidates) {
      candidates.push_back(kv.value);
    }
  }

  auto state = std::make_unique<InputStates::ChoosingCandidate>();
  state->setCursorIndex(nonEmptyState->cursorIndex());
  state->setComposingBuffer(nonEmptyState->composingBuffer());
  state->setCandidates(candidates);
  return state;
}

size_t KeyHandler::actualCandidateCursorIndex() {
  size_t cursorIndex = builder_->cursorIndex();
  if (selectPhraseAfterCursorAsCandidate_) {
    if (cursorIndex < builder_->length()) {
      ++cursorIndex;
    }
  } else {
    // Cursor must be in the middle or right after a node. So if the cursor is
    // at the beginning, move by one.
    if (!cursorIndex && builder_->length() > 0) {
      ++cursorIndex;
    }
  }
  return cursorIndex;
}

std::string KeyHandler::popEvictedTextAndWalk() {
  // in an ideal world, we can as well let the user type forever,
  // but because the Viterbi algorithm has a complexity of O(N^2),
  // the walk will become slower as the number of nodes increase,
  // therefore we need to "pop out" overflown text -- they usually
  // lose their influence over the whole MLE anyway -- so that when
  // the user type along, the already composed text at front will
  // be popped out
  // TODO(unassigned): Is the algorithm really O(n^2)? Audit.
  std::string evictedText;
  if (builder_->grid().width() > kComposingBufferSize &&
      !walkedNodes_.empty()) {
    Formosa::Gramambular::NodeAnchor& anchor = walkedNodes_[0];
    evictedText = anchor.node->currentKeyValue().value;
    builder_->removeHeadReadings(anchor.spanningLength);
  }

  walk();
  return evictedText;
}

void KeyHandler::pinNode(const std::string& candidate) {
  builder_->grid().fixNodeSelectedCandidate(actualCandidateCursorIndex(),
                                            candidate);
  walk();
}

void KeyHandler::walk() {
  // retrieve the most likely trellis, i.e. a Maximum Likelihood Estimation
  // of the best possible Mandarin characters given the input syllables,
  // using the Viterbi algorithm implemented in the Gramambular library.
  Formosa::Gramambular::Walker walker(&builder_->grid());

  // the reverse walk traces the trellis from the end
  walkedNodes_ = walker.reverseWalk(builder_->grid().width());

  // then we reverse the nodes so that we get the forward-walked nodes
  std::reverse(walkedNodes_.begin(), walkedNodes_.end());
}

}  // namespace McBopomofo
