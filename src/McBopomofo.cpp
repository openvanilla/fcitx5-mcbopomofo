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
#include <sstream>
#include <unordered_map>
#include <utility>

#include "Key.h"
#include "Log.h"
#include "UTF8Helper.h"

namespace McBopomofo {

constexpr char kConfigPath[] = "conf/mcbopomofo.conf";

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
    case FcitxKey_Tab:
      return Key::asciiKey(Key::TAB, shiftPressed, ctrlPressed);
    case FcitxKey_Left:
      return Key::namedKey(Key::KeyName::LEFT, shiftPressed, ctrlPressed);
    case FcitxKey_Right:
      return Key::namedKey(Key::KeyName::RIGHT, shiftPressed, ctrlPressed);
    case FcitxKey_Home:
      return Key::namedKey(Key::KeyName::HOME, shiftPressed, ctrlPressed);
    case FcitxKey_End:
      return Key::namedKey(Key::KeyName::END, shiftPressed, ctrlPressed);
    case FcitxKey_Up:
      return Key::namedKey(Key::KeyName::UP, shiftPressed, ctrlPressed);
    case FcitxKey_Down:
      return Key::namedKey(Key::KeyName::DOWN, shiftPressed, ctrlPressed);
    default:
      break;
  }
  return {};
}

class McBopomofoCandidateWord : public fcitx::CandidateWord {
 public:
  McBopomofoCandidateWord(fcitx::Text displayText,
                          InputStates::ChoosingCandidate::Candidate candidate,
                          std::shared_ptr<KeyHandler> keyHandler,
                          KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        candidate_(std::move(candidate)),
        keyHandler_(std::move(keyHandler)),
        stateCallback_(std::move(callback)) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    keyHandler_->candidateSelected(candidate_, stateCallback_);
  }

 private:
  InputStates::ChoosingCandidate::Candidate candidate_;
  std::shared_ptr<KeyHandler> keyHandler_;
  KeyHandler::StateCallback stateCallback_;
  std::function<void(const std::string&)> callback_;
};

class McBopomofoDictionaryServiceWord : public fcitx::CandidateWord {
 public:
  McBopomofoDictionaryServiceWord(
      fcitx::Text displayText, size_t index,
      InputStates::SelectingDictionary* currentState,
      std::shared_ptr<KeyHandler> keyHandler,
      KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        index_(index),
        currentState_(currentState),
        keyHandler_(std::move(keyHandler)),
        stateCallback_(std::move(callback)) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    keyHandler_->dictionaryServiceSelected(
        currentState_->selectedPhrase, index_, currentState_, stateCallback_);
  }

 private:
  size_t index_;
  InputStates::SelectingDictionary* currentState_;
  std::shared_ptr<KeyHandler> keyHandler_;
  KeyHandler::StateCallback stateCallback_;
  std::function<void(const std::string&)> callback_;
};

class McBopomofoTextOnlyCandidateWord : public fcitx::CandidateWord {
 public:
  McBopomofoTextOnlyCandidateWord(fcitx::Text displayText)
      : fcitx::CandidateWord(std::move(displayText)) {}
  void select(fcitx::InputContext* /*unused*/) const override {}
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

class LanguageModelLoaderLocalizedStrings
    : public LanguageModelLoader::LocalizedStrings {
 public:
  std::string userPhraseFileHeader() override {
    std::stringstream sst;
    // clang-format off
    sst
      << _("# Custom Phrases or Characters.") << "\n"
      << "#\n"
      << _("# See https://github.com/openvanilla/McBopomofo/wiki/使用手冊#手動加詞 for usage.") << "\n"
      << "#\n"
      << _("# Add your phrases and their respective Bopomofo reading below. Use hyphen (\"-\")") << "\n"
      << _("# to connect the Bopomofo syllables.") << "\n"
      << "#\n"
      << "#   小麥注音 ㄒㄧㄠˇ-ㄇㄞˋ-ㄓㄨˋ-ㄧㄣ" << "\n"
      << "#\n"
      << _("# Any line that starts with \"#\" is treated as comment.") << "\n"
      << "\n";
    // clang-format on
    return sst.str();
  }
  std::string excludedPhraseFileHeader() override {
    std::stringstream sst;
    // clang-format off
    sst
      << _("# Custom Excluded Phrases or Characters.") << "\n"
      << "#\n"
      << _("# See https://github.com/openvanilla/McBopomofo/wiki/使用手冊#手動刪詞 for usage.") << "\n"
      << "#\n"
      << _("# For example, the line below will prevent the phrase \"家祠\" from showing up anywhere:") << "\n"
      << "#\n"
      << "#   家祠 ㄐㄧㄚ-ㄘˊ" << "\n"
      << "#\n"
      << _("# Note that you need to use a hyphen (\"-\") between Bopomofo syllables.") << "\n"
      << "#\n"
      << _("# Any line that starts with \"#\" is treated as comment.") << "\n"
      << "\n";
    // clang-format on
    return sst.str();
  }
};

static std::string GetOpenFileWith(const McBopomofoConfig& config) {
  return config.openUserPhraseFilesWith.value().length() > 0
             ? config.openUserPhraseFilesWith.value()
             : kDefaultOpenFileWith;
}

McBopomofoEngine::McBopomofoEngine(fcitx::Instance* instance)
    : instance_(instance) {
  languageModelLoader_ = std::make_shared<LanguageModelLoader>(
      std::make_unique<LanguageModelLoaderLocalizedStrings>());
  keyHandler_ = std::make_shared<KeyHandler>(
      languageModelLoader_->getLM(), languageModelLoader_,
      std::make_unique<KeyHandlerLocalizedString>());

  keyHandler_->setOnAddNewPhrase([this](std::string newPhrase) {
    auto addScriptHookEnabled = config_.addScriptHookEnabled.value();
    if (!addScriptHookEnabled) {
      return;
    }
    auto scriptPath = config_.addScriptHookPath.value();
    if (scriptPath.empty()) {
      scriptPath = kDefaultAddPhraseHookPath;
    }

    auto userDataPath = languageModelLoader_->userDataPath();
    fcitx::startProcess({"/bin/sh", scriptPath, std::move(newPhrase)},
                        userDataPath);
  });

  state_ = std::make_unique<InputStates::Empty>();

  editUserPhrasesAction_ = std::make_unique<fcitx::SimpleAction>();
  editUserPhrasesAction_->setShortText(_("Edit User Phrases"));
  editUserPhrasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        fcitx::startProcess({GetOpenFileWith(config_),
                             languageModelLoader_->userPhrasesPath()});
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-phrases-edit", editUserPhrasesAction_.get());

  excludedPhrasesAction_ = std::make_unique<fcitx::SimpleAction>();
  excludedPhrasesAction_->setShortText(_("Edit Excluded Phrases"));
  excludedPhrasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        fcitx::startProcess({GetOpenFileWith(config_),
                             languageModelLoader_->excludedPhrasesPath()});
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-excluded-phrases-edit", excludedPhrasesAction_.get());

  // Required by convention of fcitx5 modules to load config on its own.
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
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

void McBopomofoEngine::activate(const fcitx::InputMethodEntry& entry,
                                fcitx::InputContextEvent& event) {
  McBopomofo::InputMode mode = entry.uniqueName() == "mcbopomofo-plain"
                                   ? McBopomofo::InputMode::PlainBopomofo
                                   : McBopomofo::InputMode::McBopomofo;

  if (mode != keyHandler_->inputMode()) {
    languageModelLoader_->loadModelForMode(mode);
  }

  chttrans();

  auto* inputContext = event.inputContext();
  if (auto* action =
          instance_->userInterfaceManager().lookupAction("chttrans")) {
    inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                         action);
  }

  if (mode == McBopomofo::InputMode::McBopomofo) {
    inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                         editUserPhrasesAction_.get());
    inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                         excludedPhrasesAction_.get());
  }

  keyHandler_->setInputMode(mode);

  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  const auto* layout =
      Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
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

  keyHandler_->setCtrlEnterKeyBehavior(config_.ctrlEnterKeys.value());

  languageModelLoader_->reloadUserModelsIfNeeded();
}

void McBopomofoEngine::reset(const fcitx::InputMethodEntry& /*unused*/,
                             fcitx::InputContextEvent& event) {
  keyHandler_->reset();

  if (dynamic_cast<fcitx::FocusOutEvent*>(&event) != nullptr) {
    // If this is a FocusOutEvent, we let fcitx5 do its own clean up, and so we
    // just force the state machine to go back to the empty state. The
    // FocusOutEvent will cause the preedit buffer to be force-committed anyway.
    //
    // Note: We don't want to call enterNewState() with EmptyIgnoringPrevious
    // state because we don't want to clean the preedit ourselves (which would
    // cause nothing to be force-committed as the focus is switched, and that
    // would cause user to lose what they've entered). We don't want to call
    // enterNewState() with Empty state, either, because that would trigger
    // commit of existing preedit buffer, resulting in double commit of the same
    // text.
    state_ = std::make_unique<InputStates::Empty>();
  } else {
    enterNewState(event.inputContext(), std::make_unique<InputStates::Empty>());
  }
}

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry& /*unused*/,
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

  if (dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::SelectingDictionary*>(state_.get()) !=
          nullptr ||
      dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr) {
    // Absorb all keys when the candidate panel is on.
    keyEvent.filterAndAccept();

#ifdef USE_LEGACY_FCITX5_API
    auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
        context->inputPanel().candidateList());
#else
    auto* maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
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

    handleCandidateKeyEvent(
        context, key, maybeCandidateList,
        [this, context](std::unique_ptr<InputState> next) {
          enterNewState(context, std::move(next));
        },
        []() {
          // TODO(unassigned): beep?
        });
    if (dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) !=
            nullptr ||
        dynamic_cast<InputStates::SelectingDictionary*>(state_.get()) !=
            nullptr ||
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr) {
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      context->updatePreedit();
    }
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
    fcitx::CommonCandidateList* candidateList,
    const McBopomofo::KeyHandler::StateCallback& stateCallback,
    const McBopomofo::KeyHandler::ErrorCallback& errorCallback) {
  int idx = key.keyListIndex(selectionKeys_);
  if (idx >= 0) {
    if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
      candidateList->candidate(idx)->select(context);
#else
      candidateList->candidate(idx).select(context);
#endif
    }
    return;
  }

  bool keyIsCancel = false;

  // When pressing "?" in the candidate list, tries to look up the candidate in
  // dictionaries.
  if (key.check(FcitxKey_question)) {
    auto choosingCandidate =
        dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
    auto selectingDictionary =
        dynamic_cast<InputStates::SelectingDictionary*>(state_.get());
    auto showingCharInfo =
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get());

    if (choosingCandidate != nullptr) {
      // Enter selecting dictionary service state.
      if (keyHandler_->hasDictionaryServices()) {
#ifdef USE_LEGACY_FCITX5_API
        int page = candidateList->currentPage();
        int pageSize = candidateList->size();
        int selectedIndex = page * pageSize + candidateList->cursorIndex();
        std::string phrase =
            candidateList->candidate(selectedIndex)->text().toString();
#else
        int selectedIndex = candidateList->globalCursorIndex();
        std::string phrase =
            candidateList->candidate(selectedIndex).text().toString();
#endif
        std::unique_ptr<InputStates::ChoosingCandidate> copy =
            choosingCandidate->copy();
        auto state = keyHandler_->buildSelectingDictionaryState(
            std::move(copy), phrase, selectedIndex);
        enterNewState(context, std::move(state));
        return;
      }
    } else if (selectingDictionary != nullptr) {
      // Leave selecting dictionary service state.
      keyIsCancel = true;
    } else if (showingCharInfo != nullptr) {
      // Leave selecting dictionary service state.
      keyIsCancel = true;
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

  if (keyIsCancel || key.check(FcitxKey_Escape) ||
      key.check(FcitxKey_BackSpace)) {
    auto* showingCharInfo =
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get());
    if (showingCharInfo != nullptr) {
      auto previous = showingCharInfo->previousState.get();
      stateCallback(previous->copy());
      return;
    }

    auto* selecting =
        dynamic_cast<InputStates::SelectingDictionary*>(state_.get());
    if (selecting != nullptr) {
      auto previous = selecting->previousState.get();
      auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(previous);
      if (choosing != nullptr) {
        stateCallback(choosing->copy());

#ifdef USE_LEGACY_FCITX5_API
        auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList());
#else
        auto* maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList().get());
#endif
        size_t index = selecting->selectedCandidateIndex;
        maybeCandidateList->setGlobalCursorIndex((int)index);
        return;
      }
      auto* marking = dynamic_cast<InputStates::Marking*>(previous);
      if (marking != nullptr) {
        stateCallback(marking->copy());
      }
      return;
    }

    keyHandler_->candidatePanelCancelled(
        [this, context](std::unique_ptr<InputState> next) {
          enterNewState(context, std::move(next));
        });
    return;
  }

  fcitx::CandidateLayoutHint layoutHint = getCandidateLayoutHint();
  candidateList->setLayoutHint(layoutHint);

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

  bool result = keyHandler_->handleCandidateKeyForTraditionalBopomofoIfRequired(
      MapFcitxKey(key),
      [candidateList, context] {
        auto idx = candidateList->cursorIndex();
        if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
          candidateList->candidate(idx)->select(context);
#else
          candidateList->candidate(idx).select(context);
#endif
        }
      },
      stateCallback, errorCallback);

  if (result) {
    return;
  }

  // TODO(unassigned): All else... beep?
  errorCallback();
}

void McBopomofoEngine::enterNewState(fcitx::InputContext* context,
                                     std::unique_ptr<InputState> newState) {
  // Hold the previous state, and transfer the ownership of newState.
  std::unique_ptr<InputState> prevState = std::move(state_);
  state_ = std::move(newState);

  InputState* prevPtr = prevState.get();
  InputState* currentPtr = state_.get();

  if (auto* empty = dynamic_cast<InputStates::Empty*>(currentPtr)) {
    handleEmptyState(context, prevPtr, empty);

  } else if (auto* emptyIgnoringPrevious =
                 dynamic_cast<InputStates::EmptyIgnoringPrevious*>(
                     currentPtr)) {
    handleEmptyIgnoringPreviousState(context, prevPtr, emptyIgnoringPrevious);

    // Transition to Empty state as required by the spec: see
    // EmptyIgnoringPrevious's own definition for why.
    state_ = std::make_unique<InputStates::Empty>();
  } else if (auto* committing =
                 dynamic_cast<InputStates::Committing*>(currentPtr)) {
    handleCommittingState(context, prevPtr, committing);
  } else if (auto* inputting =
                 dynamic_cast<InputStates::Inputting*>(currentPtr)) {
    handleInputtingState(context, prevPtr, inputting);
  } else if (auto* candidates =
                 dynamic_cast<InputStates::ChoosingCandidate*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, candidates);
  } else if (auto* selecting =
                 dynamic_cast<InputStates::SelectingDictionary*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, selecting);
  } else if (auto* showingCharInfo =
                 dynamic_cast<InputStates::ShowingCharInfo*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, showingCharInfo);
  } else if (auto* marking = dynamic_cast<InputStates::Marking*>(currentPtr)) {
    handleMarkingState(context, prevPtr, marking);
  }
}

void McBopomofoEngine::handleEmptyState(fcitx::InputContext* context,
                                        InputState* prev,
                                        InputStates::Empty* /*unused*/) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (auto* notEmpty = dynamic_cast<InputStates::NotEmpty*>(prev)) {
    context->commitString(notEmpty->composingBuffer);
  }
  context->updatePreedit();
}

void McBopomofoEngine::handleEmptyIgnoringPreviousState(
    fcitx::InputContext* context, InputState* /*unused*/,
    InputStates::EmptyIgnoringPrevious* /*unused*/) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
}

void McBopomofoEngine::handleCommittingState(fcitx::InputContext* context,
                                             InputState* /*unused*/,
                                             InputStates::Committing* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (!current->text.empty()) {
    context->commitString(current->text);
  }
  context->updatePreedit();
}

void McBopomofoEngine::handleInputtingState(fcitx::InputContext* context,
                                            InputState* /*unused*/,
                                            InputStates::Inputting* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  updatePreedit(context, current);
}

void McBopomofoEngine::handleCandidatesState(fcitx::InputContext* context,
                                             InputState* /*unused*/,
                                             InputStates::NotEmpty* current) {
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
  candidateList->setPageSize(static_cast<int>(selectionKeys_.size()));

  fcitx::CandidateLayoutHint layoutHint = getCandidateLayoutHint();
  candidateList->setLayoutHint(layoutHint);

  KeyHandler::StateCallback callback =
      [this, context](std::unique_ptr<InputState> next) {
        enterNewState(context, std::move(next));
      };

  auto choosing = dynamic_cast<InputStates::ChoosingCandidate*>(current);
  auto selectingDictionary =
      dynamic_cast<InputStates::SelectingDictionary*>(current);
  auto showingCharInfo = dynamic_cast<InputStates::ShowingCharInfo*>(current);

  if (choosing != nullptr) {
    // Construct the candidate list with special care for candidates that have
    // the same values. The display text of such a candidate will be in the form
    // of "value (reading)" to help user disambiguate those candidates.

    std::unordered_map<std::string, size_t> valueCountMap;

    for (const auto& c : choosing->candidates) {
      ++valueCountMap[c.value];
    }

    for (const auto& c : choosing->candidates) {
      std::string displayText = c.value;

      if (valueCountMap[displayText] > 1) {
        displayText += " (";
        std::string reading = c.reading;
        std::replace(reading.begin(), reading.end(),
                     KeyHandler::kJoinSeparator[0], ' ');
        displayText += reading;
        displayText += ")";
      }

#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate = new McBopomofoCandidateWord(
          fcitx::Text(displayText), c, keyHandler_, callback);
      // ownership of candidate is transferred to candidateList.
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoCandidateWord>(fcitx::Text(displayText), c,
                                                    keyHandler_, callback);
      candidateList->append(std::move(candidate));
#endif
    }
  } else if (selectingDictionary != nullptr) {
    size_t index = 0;
    for (const auto& menuItem : selectingDictionary->menu) {
      std::string displayText = menuItem;

#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate = new McBopomofoDictionaryServiceWord(
          fcitx::Text(displayText), index, selectingDictionary, keyHandler_,
          callback);
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoDictionaryServiceWord>(
              fcitx::Text(displayText), index, selectingDictionary, keyHandler_,
              callback);
      candidateList->append(std::move(candidate));
#endif
      index++;
    }
  } else if (showingCharInfo != nullptr) {
    std::vector<std::string> menu;
    menu.emplace_back(fmt::format(_("UTF8 String Length: {0}"),
                                  showingCharInfo->selectedPhrase.length()));
    size_t count = CodePointCount(showingCharInfo->selectedPhrase);
    menu.emplace_back(fmt::format(_("Code Point Count: {0}"), count));

    for (const auto& menuItem : menu) {
      std::string displayText = menuItem;

#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate =
          new McBopomofoTextOnlyCandidateWord(fcitx::Text(displayText));
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoTextOnlyCandidateWord>(
              fcitx::Text(displayText));
      candidateList->append(std::move(candidate));
#endif
    }
  }

  candidateList->toCursorMovable()->nextCandidate();
  context->inputPanel().reset();
  context->inputPanel().setCandidateList(std::move(candidateList));
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

  updatePreedit(context, current);
}

void McBopomofoEngine::handleMarkingState(fcitx::InputContext* context,
                                          InputState* /*unused*/,
                                          InputStates::Marking* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  updatePreedit(context, current);
}

fcitx::CandidateLayoutHint McBopomofoEngine::getCandidateLayoutHint() const {
  fcitx::CandidateLayoutHint layoutHint = fcitx::CandidateLayoutHint::NotSet;

  if (dynamic_cast<InputStates::SelectingDictionary*>(state_.get()) !=
      nullptr) {
    return fcitx::CandidateLayoutHint::Vertical;
  }
  if (dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr) {
    return fcitx::CandidateLayoutHint::Vertical;
  }

  auto choosingCandidate =
      dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
  if (choosingCandidate != nullptr) {
    auto candidates = choosingCandidate->candidates;
    for (InputStates::ChoosingCandidate::Candidate candidate : candidates) {
      std::string value = candidate.value;
      if (McBopomofo::CodePointCount(value) > 8) {
        return fcitx::CandidateLayoutHint::Vertical;
      }
    }
  }

  switch (config_.candidateLayout.value()) {
    case McBopomofo::CandidateLayoutHint::Vertical:
      layoutHint = fcitx::CandidateLayoutHint::Vertical;
      break;
    case McBopomofo::CandidateLayoutHint::Horizontal:
      layoutHint = fcitx::CandidateLayoutHint::Horizontal;
      break;
    case McBopomofo::CandidateLayoutHint::NotSet:
      layoutHint = fcitx::CandidateLayoutHint::NotSet;
      break;
  }

  return layoutHint;
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
  if (auto* marking = dynamic_cast<InputStates::Marking*>(state)) {
    preedit.append(marking->head, normalFormat);
    preedit.append(marking->markedText, fcitx::TextFormatFlag::HighLight);
    preedit.append(marking->tail, normalFormat);
  } else {
    // Note: dynamic_cast<InputStates::Marking*> in enterNewState() ensures
    // state is not nullptr
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    preedit.append(state->composingBuffer, normalFormat);
  }
  preedit.setCursor(static_cast<int>(state->cursorIndex));

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
