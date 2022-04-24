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

#include "McBopomofo.h"

#include <fcitx-utils/i18n.h>
#include <fcitx/candidatelist.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/userinterfacemanager.h>
#include <fmt/format.h>

#include <memory>
#include <utility>
#include <vector>

#include "Key.h"
#include "Log.h"

namespace McBopomofo {

constexpr char kConfigPath[] = "conf/mcbopomofo.conf";

// This is determined from experience. Even if a user hits Ctrl-Space quickly,
// it'll take ~200 ms (so 200,000 microseconds) to switch input method context.
// 90 ms is so fast that only a quick succession of events taken place during
// FocusOut (see McBopomofoEngine::reset() below) can cause such context switch.
constexpr int64_t kIgnoreFocusOutEventThresholdMicroseconds = 90'000;  // 90 ms

static int64_t GetEpochNowInMicroseconds() {
  auto now = std::chrono::system_clock::now();
  int64_t timestamp =
      std::chrono::time_point_cast<std::chrono::microseconds>(now)
          .time_since_epoch()
          .count();
  return timestamp;
}

static Key MapFcitxKey(const fcitx::Key& key) {
  if (key.isSimple()) {
    return Key::asciiKey(key.sym(), false);
  }

  bool shiftPressed = key.states() & fcitx::KeyState::Shift;
  bool ctrlPressed = key.states() & fcitx::KeyState::Ctrl;

  if (ctrlPressed && !shiftPressed) {
    switch (key.sym()) {
      case FcitxKey_comma:
        return Key::asciiKey(',', shiftPressed, ctrlPressed);
      case FcitxKey_period:
        return Key::asciiKey('.', shiftPressed, ctrlPressed);
      case FcitxKey_1:
        return Key::asciiKey('!', shiftPressed, ctrlPressed);
      case FcitxKey_slash:
        return Key::asciiKey('/', shiftPressed, ctrlPressed);
      case FcitxKey_semicolon:
        return Key::asciiKey(';', shiftPressed, ctrlPressed);
      case FcitxKey_apostrophe:
        return Key::asciiKey('\'', shiftPressed, ctrlPressed);
      default:
        break;
    }
  }

  switch (key.sym()) {
    case FcitxKey_BackSpace:
      return Key::asciiKey(Key::BACKSPACE, shiftPressed, ctrlPressed);
    case FcitxKey_Return:
      return Key::asciiKey(Key::RETURN, shiftPressed, ctrlPressed);
    case FcitxKey_Escape:
      return Key::asciiKey(Key::ESC, shiftPressed, ctrlPressed);
    case FcitxKey_space:
      // This path is taken when Shift is pressed--no longer a "simple" key.
      return Key::asciiKey(Key::SPACE, shiftPressed, ctrlPressed);
    case FcitxKey_Delete:
      return Key::asciiKey(Key::DELETE, shiftPressed, ctrlPressed);
    case FcitxKey_Left:
      return Key::namedKey(Key::KeyName::LEFT, shiftPressed, ctrlPressed);
    case FcitxKey_Right:
      return Key::namedKey(Key::KeyName::RIGHT, shiftPressed, ctrlPressed);
    case FcitxKey_Home:
      return Key::namedKey(Key::KeyName::HOME, shiftPressed, ctrlPressed);
    case FcitxKey_End:
      return Key::namedKey(Key::KeyName::END, shiftPressed, ctrlPressed);
    default:
      break;
  }
  return Key();
}

class McBopomofoCandidateWord : public fcitx::CandidateWord {
 public:
  McBopomofoCandidateWord(fcitx::Text text,
                          std::function<void(const std::string&)> callback)
      : fcitx::CandidateWord(std::move(text)), callback_(std::move(callback)) {}

  void select(fcitx::InputContext*) const override {
    auto string = text().toStringForCommit();
    callback_(std::move(string));
  }

 private:
  std::function<void(const std::string&)> callback_;
};

class KeyHandlerLocalizedString : public KeyHandler::LocalizedStrings {
 public:
  std::string cursorIsBetweenSyllables(
      const std::string& prevReading, const std::string& nextReading) override {
    return fmt::format(_("Cursor is between syllables {0} and {1}"),
                       prevReading, nextReading);
  }

  std::string syllablesRequired(size_t syllables) override {
    return fmt::format(_("{0} syllables required"), std::to_string(syllables));
  }

  std::string syllablesMaximum(size_t syllables) override {
    return fmt::format(_("{0} syllables maximum"), std::to_string(syllables));
  }

  std::string phraseAlreadyExists() override {
    return _("phrase already exists");
  }

  std::string pressEnterToAddThePhrase() override {
    return _("press Enter to add the phrase");
  }

  std::string markedWithSyllablesAndStatus(const std::string& marked,
                                           const std::string& readingUiText,
                                           const std::string& status) override {
    return fmt::format(_("Marked: {0}, syllables: {1}, {2}"), marked,
                       readingUiText, status);
  }
};

static std::string GetOpenFileWith(const McBopomofoConfig& config) {
  return config.openUserPhraseFilesWith.value().length() > 0
             ? config.openUserPhraseFilesWith.value()
             : kDefaultOpenFileWith;
}

McBopomofoEngine::McBopomofoEngine(fcitx::Instance* instance)
    : instance_(instance) {
  languageModelLoader_ = std::make_shared<LanguageModelLoader>();
  keyHandler_ = std::make_unique<KeyHandler>(
      languageModelLoader_->getLM(), languageModelLoader_,
      std::make_unique<KeyHandlerLocalizedString>());
  state_ = std::make_unique<InputStates::Empty>();
  stateCommittedTimestampMicroseconds_ = GetEpochNowInMicroseconds();

  editUserPhreasesAction_ = std::make_unique<fcitx::SimpleAction>();
  editUserPhreasesAction_->setShortText(_("Edit User Phrases"));
  editUserPhreasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        fcitx::startProcess({GetOpenFileWith(config_),
                             languageModelLoader_->userPhrasesPath()});
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-phrases-edit", editUserPhreasesAction_.get());

  excludedPhreasesAction_ = std::make_unique<fcitx::SimpleAction>();
  excludedPhreasesAction_->setShortText(_("Edit Excluded Phrases"));
  excludedPhreasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        fcitx::startProcess({GetOpenFileWith(config_),
                             languageModelLoader_->excludedPhrasesPath()});
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-excluded-phrases-edit", excludedPhreasesAction_.get());

  // Required by convention of fcitx5 modules to load config on its own.
  reloadConfig();
}

const fcitx::Configuration* McBopomofoEngine::getConfig() const {
  return &config_;
}

void McBopomofoEngine::setConfig(const fcitx::RawConfig& config) {
  config_.load(config, true);
  fcitx::safeSaveAsIni(config_, kConfigPath);
}

void McBopomofoEngine::reloadConfig() {
  fcitx::readAsIni(config_, kConfigPath);
}

void McBopomofoEngine::activate(const fcitx::InputMethodEntry&,
                                fcitx::InputContextEvent& event) {
  chttrans();

  auto* inputContext = event.inputContext();
  if (auto* action =
          instance_->userInterfaceManager().lookupAction("chttrans")) {
    inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                         action);
  }

  inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                       editUserPhreasesAction_.get());
  inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                       excludedPhreasesAction_.get());

  auto layout = Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
  switch (config_.bopomofoKeyboardLayout.value()) {
    case BopomofoKeyboardLayout::Standard:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
      break;
    case BopomofoKeyboardLayout::Eten:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETenLayout();
      break;
    case BopomofoKeyboardLayout::Hsu:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::HsuLayout();
      break;
    case BopomofoKeyboardLayout::Et26:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETen26Layout();
      break;
    case BopomofoKeyboardLayout::HanyuPinyin:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::HanyuPinyinLayout();
      break;
    case BopomofoKeyboardLayout::IBM:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::IBMLayout();
      break;
  }
  keyHandler_->setKeyboardLayout(layout);

  keyHandler_->setSelectPhraseAfterCursorAsCandidate(
      config_.selectPhrase.value() == SelectPhrase::AfterCursor);

  keyHandler_->setMoveCursorAfterSelection(
      config_.moveCursorAfterSelection.value());

  keyHandler_->setEscKeyClearsEntireComposingBuffer(
      config_.escKeyClearsEntireComposingBuffer.value());

  keyHandler_->setPutLowercaseLettersToComposingBuffer(
      config_.shiftLetterKeys.value() == ShiftLetterKeys::PutLowercaseToBuffer);

  if (config_.ctrlEnterKeys.value() == CtrlEnterKey::Disabled) {
    keyHandler_->setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter::Disabled);
  } else if (config_.ctrlEnterKeys.value() == CtrlEnterKey::InputReading) {
    keyHandler_->setCtrlEnterKeyBehavior(
        KeyHandlerCtrlEnter::InputBpmfReadings);
  } else if (config_.ctrlEnterKeys.value() == CtrlEnterKey::InputHTMLRubyText) {
    keyHandler_->setCtrlEnterKeyBehavior(
        KeyHandlerCtrlEnter::InputHTMLRubyText);
  }

  languageModelLoader_->reloadUserModelsIfNeeded();
}

void McBopomofoEngine::reset(const fcitx::InputMethodEntry&,
                             fcitx::InputContextEvent& event) {
  // If our previous state is Inputting and it has evicted text, and reset() is
  // called soon afterwards--here we use
  // kIgnoreFocusOutEventThresholdMicroseconds to determine it--then we should
  // ignore this event. This is because some apps, like Chrome-based browsers,
  // would trigger this event when some string is committed, even if we stay in
  // the Inputting state. If we don't ignore this, it'll cause the preedit to be
  // prematurely cleared, which disrupts the user input flow.
  auto isFocusOutEvent = dynamic_cast<fcitx::FocusOutEvent*>(&event) != nullptr;
  if (isFocusOutEvent) {
    int64_t delta =
        GetEpochNowInMicroseconds() - stateCommittedTimestampMicroseconds_;
    if (delta < kIgnoreFocusOutEventThresholdMicroseconds) {
      if (auto inputting =
              dynamic_cast<InputStates::Inputting*>(state_.get())) {
        if (inputting->evictedText.length() > 0) {
          // Don't reset anything. Stay in the current Inputting state.
          return;
        }
      }
    }
  }

  keyHandler_->reset();
  enterNewState(event.inputContext(), std::make_unique<InputStates::Empty>());
}

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry&,
                                fcitx::KeyEvent& keyEvent) {
  if (!keyEvent.isInputContextEvent()) {
    return;
  }

  if (keyEvent.isRelease()) {
    return;
  }

  fcitx::InputContext* context = keyEvent.inputContext();
  fcitx::Key key = keyEvent.key();

  if (key.states() & fcitx::KeyState::Alt ||
      key.states() & fcitx::KeyState::Super ||
      key.states() & fcitx::KeyState::CapsLock) {
    return;
  }

  if (dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) != nullptr) {
    // Absorb all keys when the candidate panel is on.
    keyEvent.filterAndAccept();

#ifdef USE_LEGACY_FCITX5_API
    auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
        context->inputPanel().candidateList());
#else
    auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
        context->inputPanel().candidateList().get());
#endif
    if (maybeCandidateList == nullptr) {
      // TODO(unassigned): Just assert this.
      FCITX_MCBOPOMOFO_WARN() << "inconsistent state";
      enterNewState(context, std::make_unique<InputStates::Empty>());
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      context->updatePreedit();
      return;
    }

    handleCandidateKeyEvent(context, key, maybeCandidateList);
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    context->updatePreedit();
    return;
  }

  bool accepted = keyHandler_->handle(
      MapFcitxKey(key), state_.get(),
      [this, context](std::unique_ptr<InputState> next) {
        enterNewState(context, std::move(next));
      },
      []() {
        // TODO(unassigned): beep?
      });

  if (accepted) {
    keyEvent.filterAndAccept();
    return;
  }
}

void McBopomofoEngine::handleCandidateKeyEvent(
    fcitx::InputContext* context, fcitx::Key key,
    fcitx::CommonCandidateList* candidateList) {
  int idx = key.keyListIndex(selectionKeys_);
  if (idx >= 0) {
    if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
      std::string candidate = candidateList->candidate(idx)->text().toString();
#else
      std::string candidate = candidateList->candidate(idx).text().toString();
#endif
      keyHandler_->candidateSelected(
          candidate, [this, context](std::unique_ptr<InputState> next) {
            enterNewState(context, std::move(next));
          });
      return;
    }
  }

  if (key.check(FcitxKey_Return)) {
    idx = candidateList->cursorIndex();
    if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
      candidateList->candidate(idx)->select(context);
#else
      candidateList->candidate(idx).select(context);
#endif
    }
    return;
  }

  if (key.check(FcitxKey_Escape) || key.check(FcitxKey_BackSpace)) {
    keyHandler_->candidatePanelCancelled(
        [this, context](std::unique_ptr<InputState> next) {
          enterNewState(context, std::move(next));
        });
    return;
  }

  // Space goes to next page or wraps to the first if at the end.
  if (key.check(FcitxKey_space)) {
    if (candidateList->hasNext()) {
      candidateList->next();
      candidateList->toCursorMovable()->nextCandidate();
    } else if (candidateList->currentPage() > 0) {
      candidateList->setPage(0);
      candidateList->toCursorMovable()->nextCandidate();
    }
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  fcitx::CandidateLayoutHint layoutHint = candidateList->layoutHint();
  bool isVertical = (layoutHint == fcitx::CandidateLayoutHint::Vertical);

  if (isVertical) {
    if (key.check(fcitx::Key(FcitxKey_Down))) {
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if (key.check(fcitx::Key(FcitxKey_Up))) {
      candidateList->toCursorMovable()->prevCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if ((key.check(fcitx::Key(FcitxKey_Right)) ||
         key.check(fcitx::Key(FcitxKey_Page_Down)) ||
         key.checkKeyList(instance_->globalConfig().defaultNextPage())) &&
        candidateList->hasNext()) {
      candidateList->next();
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if ((key.check(fcitx::Key(FcitxKey_Left)) ||
         key.check(fcitx::Key(FcitxKey_Page_Up)) ||
         key.checkKeyList(instance_->globalConfig().defaultPrevPage())) &&
        candidateList->hasPrev()) {
      candidateList->prev();
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
  } else {
    if (key.check(fcitx::Key(FcitxKey_Right))) {
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if (key.check(fcitx::Key(FcitxKey_Left))) {
      candidateList->toCursorMovable()->prevCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if ((key.check(fcitx::Key(FcitxKey_Down)) ||
         key.check(fcitx::Key(FcitxKey_Page_Down)) ||
         key.checkKeyList(instance_->globalConfig().defaultNextPage())) &&
        candidateList->hasNext()) {
      candidateList->next();
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
    if ((key.check(fcitx::Key(FcitxKey_Up)) ||
         key.check(fcitx::Key(FcitxKey_Page_Up)) ||
         key.checkKeyList(instance_->globalConfig().defaultPrevPage())) &&
        candidateList->hasPrev()) {
      candidateList->prev();
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }
  }

  // TODO(unassigned): All else... beep?
}

void McBopomofoEngine::enterNewState(fcitx::InputContext* context,
                                     std::unique_ptr<InputState> newState) {
  // Hold the previous state, and transfer the ownership of newState.
  std::unique_ptr<InputState> prevState = std::move(state_);
  state_ = std::move(newState);
  stateCommittedTimestampMicroseconds_ = GetEpochNowInMicroseconds();

  InputState* prevPtr = prevState.get();
  InputState* currentPtr = state_.get();

  if (auto empty = dynamic_cast<InputStates::Empty*>(currentPtr)) {
    handleEmptyState(context, prevPtr, empty);
  } else if (auto emptyIgnoringPrevious =
                 dynamic_cast<InputStates::EmptyIgnoringPrevious*>(
                     currentPtr)) {
    handleEmptyIgnoringPreviousState(context, prevPtr, emptyIgnoringPrevious);

    // Transition to Empty state as required by the spec: see
    // EmptyIgnoringPrevious's own definition for why.
    state_ = std::make_unique<InputStates::Empty>();
  } else if (auto committing =
                 dynamic_cast<InputStates::Committing*>(currentPtr)) {
    handleCommittingState(context, prevPtr, committing);
  } else if (auto inputting =
                 dynamic_cast<InputStates::Inputting*>(currentPtr)) {
    handleInputtingState(context, prevPtr, inputting);
  } else if (auto candidates =
                 dynamic_cast<InputStates::ChoosingCandidate*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, candidates);
  } else if (auto marking = dynamic_cast<InputStates::Marking*>(currentPtr)) {
    handleMarkingState(context, prevPtr, marking);
  }
}

void McBopomofoEngine::handleEmptyState(fcitx::InputContext* context,
                                        InputState* prev, InputStates::Empty*) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (auto notEmpty = dynamic_cast<InputStates::NotEmpty*>(prev)) {
    context->commitString(notEmpty->composingBuffer);
  }
  context->updatePreedit();
}

void McBopomofoEngine::handleEmptyIgnoringPreviousState(
    fcitx::InputContext* context, InputState*,
    InputStates::EmptyIgnoringPrevious*) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
}

void McBopomofoEngine::handleCommittingState(fcitx::InputContext* context,
                                             InputState*,
                                             InputStates::Committing* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (!current->text.empty()) {
    context->commitString(current->text);
  }
  context->updatePreedit();
}

void McBopomofoEngine::handleInputtingState(fcitx::InputContext* context,
                                            InputState*,
                                            InputStates::Inputting* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

  if (!current->evictedText.empty()) {
    context->commitString(current->evictedText);
  }
  updatePreedit(context, current);
}

void McBopomofoEngine::handleCandidatesState(
    fcitx::InputContext* context, InputState*,
    InputStates::ChoosingCandidate* current) {
  std::unique_ptr<fcitx::CommonCandidateList> candidateList =
      std::make_unique<fcitx::CommonCandidateList>();

  auto keysConfig = config_.selectionKeys.value();
  selectionKeys_.clear();

  if (keysConfig == SelectionKeys::Key_asdfghjkl) {
    selectionKeys_.emplace_back(FcitxKey_a);
    selectionKeys_.emplace_back(FcitxKey_s);
    selectionKeys_.emplace_back(FcitxKey_d);
    selectionKeys_.emplace_back(FcitxKey_f);
    selectionKeys_.emplace_back(FcitxKey_g);
    selectionKeys_.emplace_back(FcitxKey_h);
    selectionKeys_.emplace_back(FcitxKey_j);
    selectionKeys_.emplace_back(FcitxKey_k);
    selectionKeys_.emplace_back(FcitxKey_l);
  } else if (keysConfig == SelectionKeys::Key_asdfzxcvb) {
    selectionKeys_.emplace_back(FcitxKey_a);
    selectionKeys_.emplace_back(FcitxKey_s);
    selectionKeys_.emplace_back(FcitxKey_d);
    selectionKeys_.emplace_back(FcitxKey_f);
    selectionKeys_.emplace_back(FcitxKey_z);
    selectionKeys_.emplace_back(FcitxKey_x);
    selectionKeys_.emplace_back(FcitxKey_c);
    selectionKeys_.emplace_back(FcitxKey_v);
    selectionKeys_.emplace_back(FcitxKey_b);
  } else {
    selectionKeys_.emplace_back(FcitxKey_1);
    selectionKeys_.emplace_back(FcitxKey_2);
    selectionKeys_.emplace_back(FcitxKey_3);
    selectionKeys_.emplace_back(FcitxKey_4);
    selectionKeys_.emplace_back(FcitxKey_5);
    selectionKeys_.emplace_back(FcitxKey_6);
    selectionKeys_.emplace_back(FcitxKey_7);
    selectionKeys_.emplace_back(FcitxKey_8);
    selectionKeys_.emplace_back(FcitxKey_9);
  }

  candidateList->setSelectionKey(selectionKeys_);
  candidateList->setPageSize(selectionKeys_.size());

  for (const std::string& candidateStr : current->candidates) {
#ifdef USE_LEGACY_FCITX5_API
    fcitx::CandidateWord* candidate = new McBopomofoCandidateWord(
        fcitx::Text(candidateStr),
        [this, context](const std::string& candidate) {
          keyHandler_->candidateSelected(
              candidate, [this, context](std::unique_ptr<InputState> next) {
                enterNewState(context, std::move(next));
              });
        });
    // ownership of candidate is transferred to candidateList.
    candidateList->append(candidate);
#else
    std::unique_ptr<fcitx::CandidateWord> candidate =
        std::make_unique<McBopomofoCandidateWord>(
            fcitx::Text(candidateStr),
            [this, context](const std::string& candidate) {
              keyHandler_->candidateSelected(
                  candidate, [this, context](std::unique_ptr<InputState> next) {
                    enterNewState(context, std::move(next));
                  });
            });
    candidateList->append(std::move(candidate));
#endif
  }
  candidateList->toCursorMovable()->nextCandidate();
  context->inputPanel().reset();
  context->inputPanel().setCandidateList(std::move(candidateList));
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

  updatePreedit(context, current);
}

void McBopomofoEngine::handleMarkingState(fcitx::InputContext* context,
                                          InputState*,
                                          InputStates::Marking* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  updatePreedit(context, current);
}

void McBopomofoEngine::updatePreedit(fcitx::InputContext* context,
                                     InputStates::NotEmpty* state) {
  bool useClientPreedit =
      context->capabilityFlags().test(fcitx::CapabilityFlag::Preedit);
#ifdef USE_LEGACY_FCITX5_API
  fcitx::TextFormatFlags normalFormat{useClientPreedit
                                          ? fcitx::TextFormatFlag::Underline
                                          : fcitx::TextFormatFlag::None};
#else
  fcitx::TextFormatFlags normalFormat{useClientPreedit
                                          ? fcitx::TextFormatFlag::Underline
                                          : fcitx::TextFormatFlag::NoFlag};
#endif
  fcitx::Text preedit;
  if (auto marking = dynamic_cast<InputStates::Marking*>(state)) {
    preedit.append(marking->head, normalFormat);
    preedit.append(marking->markedText, fcitx::TextFormatFlag::HighLight);
    preedit.append(marking->tail, normalFormat);
  } else {
    preedit.append(state->composingBuffer, normalFormat);
  }
  preedit.setCursor(state->cursorIndex);

  if (useClientPreedit) {
    context->inputPanel().setClientPreedit(preedit);
  } else {
    context->inputPanel().setPreedit(preedit);
  }

  context->inputPanel().setAuxDown(fcitx::Text(state->tooltip));
  context->updatePreedit();
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

}  // namespace McBopomofo
