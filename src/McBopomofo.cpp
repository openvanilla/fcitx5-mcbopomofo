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
#include <notifications_public.h>  // from fcitx-module/notifications

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Format.h"
#include "Key.h"
#include "Log.h"
#include "UTF8Helper.h"

namespace McBopomofo {

constexpr char kConfigPath[] = "conf/mcbopomofo.conf";

// These two are used to determine whether Shift-[1-9] is pressed.
constexpr int kFcitxRawKeycode_1 = 10;
constexpr int kFcitxRawKeycode_9 = 18;

// For determining whether Shift-Enter is pressed in the candidate panel.
constexpr int kFcitxRawKeycode_Enter = 36;

// Fctix5 notification timeout, in milliseconds.
constexpr int32_t kFcitx5NotificationTimeoutInMs = 1000;

// If a horizontal panel contains a candidate that's longer than this number,
// the panel will be changed to a vertical panel.
constexpr size_t kForceVerticalCandidateThreshold = 8;

static Key MapFcitxKey(const fcitx::Key& key, const fcitx::Key& origKey) {
  bool shiftPressed = key.states() & fcitx::KeyState::Shift;
  bool ctrlPressed = key.states() & fcitx::KeyState::Ctrl;

  // CapsLock state only exists in the original key, so we have to use the
  // origKey variable to check for the state.
  if (origKey.states() & fcitx::KeyState::CapsLock) {
    // When we enter the branch, it means the user want to input Chinese using
    // Bopomofo even when Caps Lock is on, so we swap the uppercase and
    // lowercase letters here.
    fcitx::KeySym sym = key.sym();
    if (sym >= 'A' && sym <= 'Z') {
      return Key::asciiKey(sym + 'a' - 'A',
                           key.states() & fcitx::KeyState::Shift,
                           key.states() & fcitx::KeyState::Ctrl);
    }
    if (sym >= 'a' && sym <= 'z') {
      return Key::asciiKey(sym + 'A' - 'a',
                           key.states() & fcitx::KeyState::Shift,
                           key.states() & fcitx::KeyState::Ctrl);
    }
  }

  if (key.isSimple()) {
    return Key::asciiKey(key.sym(), false);
  }

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
      case FcitxKey_backslash:
        return Key::asciiKey('\\', shiftPressed, ctrlPressed);
      default:
        break;
    }
  }

  switch (key.sym()) {
    case FcitxKey_BackSpace:
      return Key::asciiKey(Key::BACKSPACE, shiftPressed, ctrlPressed);
    case FcitxKey_Return:
      return Key::asciiKey(Key::RETURN, shiftPressed, ctrlPressed);
    case FcitxKey_KP_Enter:
      return Key::asciiKey(Key::RETURN, shiftPressed, ctrlPressed, true);
    case FcitxKey_Escape:
      return Key::asciiKey(Key::ESC, shiftPressed, ctrlPressed);
    case FcitxKey_space:
      // This path is taken when Shift is pressed--no longer a "simple" key.
      return Key::asciiKey(Key::SPACE, shiftPressed, ctrlPressed);
    case FcitxKey_Delete:
      return Key::asciiKey(Key::DELETE, shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Delete:
      return Key::asciiKey(Key::DELETE, shiftPressed, ctrlPressed);
    case FcitxKey_Tab:
      return Key::asciiKey(Key::TAB, shiftPressed, ctrlPressed);
    case FcitxKey_Left:
      return Key::namedKey(Key::KeyName::LEFT, shiftPressed, ctrlPressed);
    case FcitxKey_KP_Left:
      return Key::namedKey(Key::KeyName::LEFT, shiftPressed, ctrlPressed, true);
    case FcitxKey_Right:
      return Key::namedKey(Key::KeyName::RIGHT, shiftPressed, ctrlPressed);
    case FcitxKey_KP_Right:
      return Key::namedKey(Key::KeyName::RIGHT, shiftPressed, ctrlPressed,
                           true);
    case FcitxKey_Home:
      return Key::namedKey(Key::KeyName::HOME, shiftPressed, ctrlPressed);
    case FcitxKey_KP_Home:
      return Key::namedKey(Key::KeyName::HOME, shiftPressed, ctrlPressed, true);
    case FcitxKey_End:
      return Key::namedKey(Key::KeyName::END, shiftPressed, ctrlPressed);
    case FcitxKey_KP_End:
      return Key::namedKey(Key::KeyName::END, shiftPressed, ctrlPressed, true);
    case FcitxKey_Up:
      return Key::namedKey(Key::KeyName::UP, shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Up:
      return Key::namedKey(Key::KeyName::UP, shiftPressed, ctrlPressed);
    case FcitxKey_Down:
      return Key::namedKey(Key::KeyName::DOWN, shiftPressed, ctrlPressed);
    case FcitxKey_KP_Down:
      return Key::namedKey(Key::KeyName::DOWN, shiftPressed, ctrlPressed, true);
    default:
      break;
  }

  switch (key.sym()) {
    case FcitxKey_KP_0:
      return Key::asciiKey('0', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_1:
      return Key::asciiKey('1', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_2:
      return Key::asciiKey('2', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_3:
      return Key::asciiKey('3', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_4:
      return Key::asciiKey('4', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_5:
      return Key::asciiKey('5', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_6:
      return Key::asciiKey('6', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_7:
      return Key::asciiKey('7', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_8:
      return Key::asciiKey('8', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_9:
      return Key::asciiKey('4', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Decimal:
      return Key::asciiKey('.', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Add:
      return Key::asciiKey('+', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Subtract:
      return Key::asciiKey('-', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Multiply:
      return Key::asciiKey('*', shiftPressed, ctrlPressed, true);
    case FcitxKey_KP_Divide:
      return Key::asciiKey('/', shiftPressed, ctrlPressed, true);

    default:
      break;
  }

  return Key{};
}

// The candidate word for the standard candidates.
class McBopomofoCandidateWord : public fcitx::CandidateWord {
 public:
  McBopomofoCandidateWord(fcitx::Text displayText,
                          InputStates::ChoosingCandidate::Candidate candidate,
                          size_t originalCursor,
                          std::shared_ptr<KeyHandler> keyHandler,
                          KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        candidate_(std::move(candidate)),
        originalCursor_(originalCursor),
        keyHandler_(std::move(keyHandler)),
        stateCallback_(std::move(callback)) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    keyHandler_->candidateSelected(candidate_, originalCursor_, stateCallback_);
  }

 private:
  InputStates::ChoosingCandidate::Candidate candidate_;
  size_t originalCursor_;
  std::shared_ptr<KeyHandler> keyHandler_;
  KeyHandler::StateCallback stateCallback_;
};

// The candidate word for the associated phrases for McBopomofo mode.
class McBopomofoAssociatedPhraseCandidateWord : public fcitx::CandidateWord {
 public:
  McBopomofoAssociatedPhraseCandidateWord(
      fcitx::Text displayText,
      InputStates::ChoosingCandidate::Candidate associatedPhraseCandidate,
      std::string overrideReading, std::string overrideValue,
      size_t cursorIndex, std::shared_ptr<KeyHandler> keyHandler,
      KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        associatedPhraseCandidate_(std::move(associatedPhraseCandidate)),
        keyHandler_(std::move(keyHandler)),
        stateCallback_(std::move(callback)),
        overrideReading_(std::move(overrideReading)),
        overrideValue_(std::move(overrideValue)),
        cursorIndex_(cursorIndex) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    keyHandler_->candidateAssociatedPhraseSelected(
        cursorIndex_, associatedPhraseCandidate_, overrideReading_,
        overrideValue_, stateCallback_);
  }

 private:
  InputStates::ChoosingCandidate::Candidate associatedPhraseCandidate_;
  std::shared_ptr<KeyHandler> keyHandler_;
  KeyHandler::StateCallback stateCallback_;
  std::string overrideReading_;
  std::string overrideValue_;
  size_t cursorIndex_;
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
};

class McBopomofoFeatureWord : public fcitx::CandidateWord {
 public:
  McBopomofoFeatureWord(fcitx::Text displayText, size_t index,
                        InputStates::SelectingFeature* currentState,
                        std::shared_ptr<KeyHandler> keyHandler,
                        KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        index_(index),
        currentState_(currentState),
        keyHandler_(std::move(keyHandler)),
        stateCallback_(std::move(callback)) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    auto nextState = currentState_->nextState(index_);
    stateCallback_(std::move(nextState));
  }

 private:
  size_t index_;
  InputStates::SelectingFeature* currentState_;
  std::shared_ptr<KeyHandler> keyHandler_;
  KeyHandler::StateCallback stateCallback_;
};

class McBopomofoCustomMenuWord : public fcitx::CandidateWord {
 public:
  explicit McBopomofoCustomMenuWord(fcitx::Text displayText, size_t index,
                                    InputStates::CustomMenu* currentState)
      : fcitx::CandidateWord(std::move(displayText)),
        index_(index),
        currentState_(currentState) {}

  void select(fcitx::InputContext* /*unused*/) const override {
    // callback(std::make_unique<InputStates::Committing>(text));
    auto entry = currentState_->entries[index_];
    if (entry.callback) {
      entry.callback();
    }
  }

 private:
  size_t index_;
  InputStates::CustomMenu* currentState_;
};

class McBopomofoDirectInsertWord : public fcitx::CandidateWord {
 public:
  explicit McBopomofoDirectInsertWord(fcitx::Text displayText, std::string text,
                                      KeyHandler::StateCallback callback)
      : fcitx::CandidateWord(std::move(displayText)),
        text(std::move(text)),
        callback(std::move(callback)) {}
  void select(fcitx::InputContext* /*unused*/) const override {
    callback(std::make_unique<InputStates::Committing>(text));
  }

  std::string text;
  KeyHandler::StateCallback callback;
};

class McBopomofoTextOnlyCandidateWord : public fcitx::CandidateWord {
 public:
  explicit McBopomofoTextOnlyCandidateWord(fcitx::Text displayText)
      : fcitx::CandidateWord(std::move(displayText)) {}
  void select(fcitx::InputContext* /*unused*/) const override {}
};

class KeyHandlerLocalizedString : public KeyHandler::LocalizedStrings {
 public:
  std::string cursorIsBetweenSyllables(
      const std::string& prevReading, const std::string& nextReading) override {
    return fmt::format(FmtRuntime(_("Cursor is between syllables {0} and {1}")),
                       prevReading, nextReading);
  }

  std::string syllablesRequired(size_t syllables) override {
    return fmt::format(FmtRuntime(_("{0} syllables required")),
                       std::to_string(syllables));
  }

  std::string syllablesMaximum(size_t syllables) override {
    return fmt::format(FmtRuntime(_("{0} syllables maximum")),
                       std::to_string(syllables));
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
    return fmt::format(FmtRuntime(_("Marked: {0}, syllables: {1}, {2}")),
                       marked, readingUiText, status);
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
  return !config.openUserPhraseFilesWith.value().empty()
             ? config.openUserPhraseFilesWith.value()
             : kDefaultOpenFileWith;
}

McBopomofoEngine::McBopomofoEngine(fcitx::Instance* instance)
    : instance_(instance) {
  languageModelLoader_ = std::make_shared<LanguageModelLoader>(
      std::make_unique<LanguageModelLoaderLocalizedStrings>());
  userFileIssues_ = languageModelLoader_->getUserFileIssues();
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

  halfWidthPunctuationAction_ = std::make_unique<fcitx::SimpleAction>();
  halfWidthPunctuationAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext* context) {
        bool enabled = config_.halfWidthPunctuationEnable.value();
        enabled = !enabled;
        config_.halfWidthPunctuationEnable.setValue(enabled);
        keyHandler_->setHalfWidthPunctuationEnabled(enabled);
        fcitx::safeSaveAsIni(config_, kConfigPath);
        halfWidthPunctuationAction_->setShortText(
            config_.halfWidthPunctuationEnable.value()
                ? _("Half Width Punctuation")
                : _("Full Width Punctuation"));
        halfWidthPunctuationAction_->update(context);

        if (notifications()) {
          notifications()->call<fcitx::INotifications::showTip>(
              "mcbopomofo-half-width-punctuation-toggle", _("McBopomofo"),
              "fcitx_mcbopomofo",
              enabled ? _("Half Width Punctuation")
                      : _("Full Width Punctuation"),
              enabled ? _("Now using half width punctuation")
                      : _("Now using full width punctuation"),
              kFcitx5NotificationTimeoutInMs);
        }
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-half-width-punctuation", halfWidthPunctuationAction_.get());

  associatedPhrasesAction_ = std::make_unique<fcitx::SimpleAction>();
  associatedPhrasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext* context) {
        bool enabled = config_.associatedPhrasesEnabled.value();
        enabled = !enabled;
        config_.associatedPhrasesEnabled.setValue(enabled);
        keyHandler_->setAssociatedPhrasesEnabled(enabled);
        fcitx::safeSaveAsIni(config_, kConfigPath);
        associatedPhrasesAction_->setShortText(
            config_.associatedPhrasesEnabled.value()
                ? _("Associated Phrases - On")
                : _("Associated Phrases - Off"));
        associatedPhrasesAction_->update(context);

        if (notifications()) {
          auto mode = keyHandler_->inputMode();
          notifications()->call<fcitx::INotifications::showTip>(
              "mcbopomofo-associated-phrases-toggle", _("McBopomofo"),
              "fcitx_mcbopomofo",
              enabled ? _("Associated Phrases On")
                      : _("Associated Phrases Off"),
              enabled ? mode == InputMode::McBopomofo
                            ? _("Now you can use Shift + Enter to insert "
                                "associated phrases")
                            : _("Associated Phrases is now enabled.")
                      : _("Associated Phrases is now disabled."),
              kFcitx5NotificationTimeoutInMs);
        }
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-associated-phrases", associatedPhrasesAction_.get());

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

  halfWidthPunctuationAction_->setShortText(
      config_.halfWidthPunctuationEnable.value() ? _("Half width Punctuation")
                                                 : _("Full Width Punctuation"));
  halfWidthPunctuationAction_->update(inputContext);
  inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                       halfWidthPunctuationAction_.get());

  associatedPhrasesAction_->setShortText(
      config_.associatedPhrasesEnabled.value() ? _("Associated Phrases - On")
                                               : _("Associated Phrases - Off"));
  associatedPhrasesAction_->update(inputContext);
  inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                       associatedPhrasesAction_.get());

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
  keyHandler_->setShiftEnterEnabled(config_.shiftEnterEnabled.value());
  keyHandler_->setCtrlEnterKeyBehavior(config_.ctrlEnterKeys.value());
  keyHandler_->setAssociatedPhrasesEnabled(
      config_.associatedPhrasesEnabled.value());
  keyHandler_->setHalfWidthPunctuationEnabled(
      config_.halfWidthPunctuationEnable.value());
  keyHandler_->setRepeatedPunctuationToSelectCandidateEnabled(
      config_.repeatedPunctuationToSelectCandidateEnabled.value());
  keyHandler_->setChooseCandidateUsingSpace(
      config_.chooseCandidateUsingSpace.value());

  bool didReload = languageModelLoader_->reloadUserModelsIfNeeded();
  if (didReload) {
    userFileIssues_ = languageModelLoader_->getUserFileIssues();
  }

  if (!userFileIssues_.empty()) {
    showAndClearUserFileIssues();
  }
}

void McBopomofoEngine::reset(const fcitx::InputMethodEntry& /*unused*/,
                             fcitx::InputContextEvent& event) {
  keyHandler_->reset();

  if (event.type() == fcitx::EventType::InputContextFocusOut ||
      event.type() == fcitx::EventType::InputContextReset) {
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
  fcitx::Key origKey = keyEvent.rawKey();

  if (key.states() & fcitx::KeyState::Alt ||
      key.states() & fcitx::KeyState::Super) {
    return;
  }

  // CapsLock is a special case. The state only exists in the raw key, not in
  // the key.
  if (origKey.states() & fcitx::KeyState::CapsLock) {
    if (!config_.capsLockAllowChineseInput.value()) {
      keyHandler_->reset();
      enterNewState(context, std::make_unique<InputStates::Empty>());
      return;
    }
  }

  if (dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::SelectingDictionary*>(state_.get()) !=
          nullptr ||
      dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::AssociatedPhrases*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state_.get()) !=
          nullptr ||
      dynamic_cast<InputStates::SelectingFeature*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::SelectingDateMacro*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::CustomMenu*>(state_.get()) != nullptr) {
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

    bool handled = handleCandidateKeyEvent(
        context, key, origKey, maybeCandidateList,
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
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr ||
        dynamic_cast<InputStates::AssociatedPhrases*>(state_.get()) !=
            nullptr ||
        dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state_.get()) !=
            nullptr ||
        dynamic_cast<InputStates::SelectingFeature*>(state_.get()) != nullptr ||
        dynamic_cast<InputStates::SelectingDateMacro*>(state_.get()) !=
            nullptr ||
        dynamic_cast<InputStates::CustomMenu*>(state_.get()) != nullptr) {
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      context->updatePreedit();
    }
    if (handled) {
      return;
    }
  }

  bool accepted = keyHandler_->handle(
      MapFcitxKey(key, origKey), state_.get(),
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

bool McBopomofoEngine::handleCandidateKeyEvent(
    fcitx::InputContext* context, fcitx::Key key, fcitx::Key origKey,
    fcitx::CommonCandidateList* candidateList,
    const McBopomofo::KeyHandler::StateCallback& stateCallback,
    const McBopomofo::KeyHandler::ErrorCallback& errorCallback) {
  InputStates::AssociatedPhrases* associatedPhrases =
      dynamic_cast<InputStates::AssociatedPhrases*>(state_.get());
  InputStates::AssociatedPhrasesPlain* associatedPhrasesPlain =
      dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state_.get());
  bool shouldUseShiftKey =
      associatedPhrasesPlain != nullptr ||
      (associatedPhrases != nullptr && associatedPhrases->useShiftKey &&
       config_.shiftEnterEnabled.value());

  // Plain Bopomofo and Associated Phrases.
  if (shouldUseShiftKey) {
    int code = origKey.code();
    // Shift-[1-9] keys can only be checked via raw key codes. The Key objects
    // in the selectionKeys_ do not carry such information.
    if ((origKey.states() & fcitx::KeyState::Shift) &&
        code >= kFcitxRawKeycode_1 && code <= kFcitxRawKeycode_9) {
      int idx = code - kFcitxRawKeycode_1;
      if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
        candidateList->candidate(idx)->select(context);
#else
        candidateList->candidate(idx).select(context);
#endif
      }
      return true;
    }
  } else {
    // handle num pad.

    int idx = key.keyListIndex(selectionKeys_);
    if (idx == -1) {
      idx = key.keyListIndex(numpadSelectionKeys_);
    }

    if (idx != -1 && idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
      candidateList->candidate(idx)->select(context);
#else
      candidateList->candidate(idx).select(context);
#endif
      return true;
    }
  }

  bool isCursorMovingLeft =
      key.check(FcitxKey_Left, fcitx::KeyStates(fcitx::KeyState::Shift));
  bool isCursorMovingRight =
      key.check(FcitxKey_Right, fcitx::KeyStates(fcitx::KeyState::Shift));

  if (!isCursorMovingLeft && !isCursorMovingRight) {
    MovingCursorOption movingCursorOption =
        config_.allowMovingCursorWhenChoosingCandidates.value();
    if (movingCursorOption == MovingCursorOption::UseJK) {
      isCursorMovingLeft = key.check(FcitxKey_j);
      isCursorMovingRight = key.check(FcitxKey_k);
    } else if (movingCursorOption == MovingCursorOption::UseHL) {
      isCursorMovingLeft = key.check(FcitxKey_h);
      isCursorMovingRight = key.check(FcitxKey_l);
    }
  }

  if (keyHandler_->inputMode() == McBopomofo::InputMode::McBopomofo &&
      dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) != nullptr &&
      (isCursorMovingLeft || isCursorMovingRight)) {
    size_t cursor = keyHandler_->candidateCursorIndex();

    if (isCursorMovingLeft) {
      if (cursor > 0) {
        cursor--;
      }
    } else if (isCursorMovingRight) {
      cursor++;
    }
    keyHandler_->setCandidateCursorIndex(cursor);
    auto inputting = keyHandler_->buildInputtingState();
    auto choosing = keyHandler_->buildChoosingCandidateState(
        inputting.get(), keyHandler_->candidateCursorIndex());
    stateCallback(std::move(choosing));
    return true;
  }

  bool keyIsCancel = false;

  std::vector<std::string> invalidPrefixes = {
      "_half_punctuation_", "_ctrl_punctuation_", "_letter_",
      "_number_",           "_punctuation_",
  };

  // When pressing "?" in the candidate list, tries to look up the candidate
  // in dictionaries.
  if (keyHandler_->inputMode() == McBopomofo::InputMode::McBopomofo &&
      key.check(FcitxKey_question)) {
    auto* choosingCandidate =
        dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
    auto* selectingDictionary =
        dynamic_cast<InputStates::SelectingDictionary*>(state_.get());
    auto* showingCharInfo =
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get());

    if (choosingCandidate != nullptr) {
      // Enter selecting dictionary service state.
      if (keyHandler_->hasDictionaryServices()) {
        int page = candidateList->currentPage();
        int pageSize = candidateList->size();
        int selectedCandidateIndex =
            (page * pageSize) + candidateList->cursorIndex();
        std::string reading =
            choosingCandidate->candidates[selectedCandidateIndex].reading;

        // If the reading has an invalid prefix, skip dictionary lookup
        if (std::any_of(invalidPrefixes.begin(), invalidPrefixes.end(),
                        [&reading](const std::string& prefix) {
                          return reading.compare(0, prefix.length(), prefix) ==
                                 0;
                        })) {
          return true;
        }

        std::string phrase =
            choosingCandidate->candidates[selectedCandidateIndex].value;
        std::unique_ptr<InputStates::ChoosingCandidate> copy =
            std::make_unique<InputStates::ChoosingCandidate>(
                *choosingCandidate);
        auto newState = keyHandler_->buildSelectingDictionaryState(
            std::move(copy), phrase, selectedCandidateIndex);
        stateCallback(std::move(newState));
        return true;
      }
    } else if (selectingDictionary != nullptr || showingCharInfo != nullptr) {
      // Leave selecting dictionary service state.
      keyIsCancel = true;
    }
  }

  if (keyHandler_->inputMode() == McBopomofo::InputMode::McBopomofo) {
    auto* choosingCandidate =
        dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
    bool isPlusKey = key.check(FcitxKey_plus) || key.check(FcitxKey_equal);
    bool isMinusKey =
        key.check(FcitxKey_minus) || key.check(FcitxKey_underscore);
    if (choosingCandidate != nullptr && (isPlusKey || isMinusKey)) {
      int page = candidateList->currentPage();
      int pageSize = candidateList->size();
      int index = candidateList->cursorIndex();
      int selectedCandidateIndex = (page * pageSize) + index;
      auto candidate = choosingCandidate->candidates[selectedCandidateIndex];
      std::string reading = candidate.reading;
      // If the reading has an invalid prefix, skip dictionary lookup
      if (std::any_of(invalidPrefixes.begin(), invalidPrefixes.end(),
                      [&reading](const std::string& prefix) {
                        return reading.compare(0, prefix.length(), prefix) == 0;
                      })) {
        return true;
      }

      // If the reading doesn't contain a hyphen, return true
      if (reading.find('-') == std::string::npos) {
        return true;
      }
      std::string phrase = candidate.value;
      std::string rawValue = candidate.rawValue;
      if (phrase != rawValue) {
        return true;
      }

      std::vector<InputStates::CustomMenu::MenuEntry> entries;
      std::string title;
      if (isPlusKey) {
        InputStates::CustomMenu::MenuEntry confirmEntry(
            _("Boost"), [this, phrase, reading, stateCallback]() {
              keyHandler_->boostPhrase(reading, phrase);
              auto inputting = keyHandler_->buildInputtingState();
              stateCallback(std::move(inputting));
            });
        entries.emplace_back(std::move(confirmEntry));
        title = fmt::format(
            FmtRuntime(
                _("Do you want to boost the score of the phrase \"{}\"?")),
            phrase);
      } else if (isMinusKey) {
        InputStates::CustomMenu::MenuEntry confirmEntry(
            _("Exclude"), [this, phrase, reading, stateCallback]() {
              keyHandler_->excludePhrase(reading, phrase);
              auto inputting = keyHandler_->buildInputtingState();
              stateCallback(std::move(inputting));
            });
        entries.emplace_back(std::move(confirmEntry));
        title = fmt::format(
            FmtRuntime(_("Do you want to exclude the phrase \"{}\"?")), phrase);
      }

      InputStates::CustomMenu::MenuEntry cancelEntry(
          _("Cancel"), [this, stateCallback]() {
            auto inputting = keyHandler_->buildInputtingState();
            auto choosing = keyHandler_->buildChoosingCandidateState(
                inputting.get(), keyHandler_->candidateCursorIndex());
            stateCallback(std::move(inputting));
            stateCallback(std::move(choosing));
          });
      entries.push_back(std::move(cancelEntry));
      auto copy =
          std::make_unique<InputStates::ChoosingCandidate>(*choosingCandidate);
      auto confirm = std::make_unique<InputStates::CustomMenu>(
          std::move(copy), std::move(title), std::move(entries));
      stateCallback(std::move(confirm));
      return true;
    }
  }

  if (keyHandler_->inputMode() == McBopomofo::InputMode::McBopomofo &&
      !shouldUseShiftKey && origKey.code() == kFcitxRawKeycode_Enter &&
      (origKey.states() & fcitx::KeyState::Shift)) {
    int idx = candidateList->cursorIndex();
    if (idx < candidateList->size()) {
      auto* choosingCandidate =
          dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
      if (choosingCandidate != nullptr) {
#ifdef USE_LEGACY_FCITX5_API
        size_t globalIndex =
            idx + candidateList->size() * candidateList->currentPage();
#else
        size_t globalIndex = candidateList->globalCursorIndex();
#endif
        auto prevState = std::make_unique<InputStates::ChoosingCandidate>(
            *choosingCandidate);
        std::string prefixReading =
            choosingCandidate->candidates[globalIndex].reading;
        std::string prefixValue =
            choosingCandidate->candidates[globalIndex].value;

        std::unique_ptr<InputStates::AssociatedPhrases> newState =
            keyHandler_->buildAssociatedPhrasesStateFromCandidateChoosingState(
                std::move(prevState), choosingCandidate->originalCursor,
                prefixReading, prefixValue, globalIndex);
        if (newState != nullptr) {
          stateCallback(std::move(newState));
        }
        return true;
      }
    }
  }

  bool returnPressed = false;
  if (shouldUseShiftKey) {
    // If it is the associated phrases mode, we need to check if the shift key
    // is pressed.
    returnPressed =
        key.check(FcitxKey_Return, fcitx::KeyStates(fcitx::KeyState::Shift));
  } else {
    // Otherwise, we just check if the return key is pressed.
    returnPressed = key.check(FcitxKey_Return);
  }

  if (returnPressed) {
    int idx = candidateList->cursorIndex();
    if (idx < candidateList->size()) {
#ifdef USE_LEGACY_FCITX5_API
      candidateList->candidate(idx)->select(context);
#else
      candidateList->candidate(idx).select(context);
#endif
    }
    return true;
  }

  if (keyIsCancel || key.check(FcitxKey_Escape) ||
      key.check(FcitxKey_BackSpace)) {
    auto* showingCharInfo =
        dynamic_cast<InputStates::ShowingCharInfo*>(state_.get());
    if (showingCharInfo != nullptr) {
      auto* previous = showingCharInfo->previousState.get();
      auto copy = std::make_unique<InputStates::SelectingDictionary>(*previous);
      stateCallback(std::move(copy));
      return true;
    }

    auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(state_.get());
    if (customMenu != nullptr) {
      auto* choosingCandidate = dynamic_cast<InputStates::ChoosingCandidate*>(
          customMenu->previousState.get());
      if (choosingCandidate != nullptr) {
        auto copy = std::make_unique<InputStates::ChoosingCandidate>(
            *choosingCandidate);
        stateCallback(std::move(copy));
      }
      return true;
    }

    auto* selecting =
        dynamic_cast<InputStates::SelectingDictionary*>(state_.get());
    if (selecting != nullptr) {
      auto* previous = selecting->previousState.get();
      auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(previous);
      if (choosing != nullptr) {
        auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
        stateCallback(std::move(copy));

#ifdef USE_LEGACY_FCITX5_API
        auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList());
#else
        auto* maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList().get());
#endif
        size_t index = selecting->selectedCandidateIndex;
        // Make sure fcitx5 shows the candidate page; needed when page > 0.
#ifndef USE_LEGACY_FCITX5_API
        maybeCandidateList->setPage(static_cast<int>(index) /
                                    maybeCandidateList->pageSize());
#endif
        maybeCandidateList->setGlobalCursorIndex(static_cast<int>(index));
        return true;
      }
      auto* marking = dynamic_cast<InputStates::Marking*>(previous);
      if (marking != nullptr) {
        auto copy = std::make_unique<InputStates::Marking>(*marking);
        stateCallback(std::move(copy));
      }
      return true;
    }

    if (associatedPhrases != nullptr) {
      if (associatedPhrases->useShiftKey) {
        return false;
      }
      auto* previous = associatedPhrases->previousState.get();
      auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(previous);
      auto* inputting = dynamic_cast<InputStates::Inputting*>(previous);
      if (choosing != nullptr) {
        auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
        stateCallback(std::move(copy));

#ifdef USE_LEGACY_FCITX5_API
        auto maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList());
#else
        auto* maybeCandidateList = dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList().get());
#endif
        size_t index = associatedPhrases->selectedCandidateIndex;
#ifndef USE_LEGACY_FCITX5_API
        // Make sure fcitx5 shows the candidate page; needed when page > 0.
        maybeCandidateList->setPage(static_cast<int>(index) /
                                    maybeCandidateList->pageSize());
#endif
        maybeCandidateList->setGlobalCursorIndex(static_cast<int>(index));
      } else if (inputting != nullptr) {
        auto copy = std::make_unique<InputStates::Inputting>(*inputting);
        stateCallback(std::move(copy));
      }
      return true;
    }

    size_t originalCursor = 0;
    auto* choosing =
        dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
    if (choosing != nullptr) {
      originalCursor = choosing->originalCursor;
    }

    keyHandler_->candidatePanelCancelled(
        originalCursor, [stateCallback](std::unique_ptr<InputState> next) {
          stateCallback(std::move(next));
        });
    return true;
  }

  fcitx::CandidateLayoutHint layoutHint = getCandidateLayoutHint();
  candidateList->setLayoutHint(layoutHint);

  // Space goes to next page or wraps to the first if at the end.
  if (key.check(FcitxKey_space)) {
    if (associatedPhrases != nullptr) {
      if (associatedPhrases->useShiftKey) {
        return false;
      }
    }

    if (candidateList->hasNext()) {
      candidateList->next();
      candidateList->toCursorMovable()->nextCandidate();
    } else if (candidateList->currentPage() > 0) {
      candidateList->setPage(0);
      candidateList->toCursorMovable()->nextCandidate();
    }
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return true;
  }

  bool isVertical = (layoutHint == fcitx::CandidateLayoutHint::Vertical);

  if (isVertical) {
    if (key.check(fcitx::Key(FcitxKey_Down))) {
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Up))) {
      candidateList->toCursorMovable()->prevCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Right)) ||
        key.check(fcitx::Key(FcitxKey_Page_Down)) ||
        key.check(fcitx::Key(FcitxKey_KP_Right)) ||
        key.check(fcitx::Key(FcitxKey_KP_Page_Down)) ||
        key.checkKeyList(instance_->globalConfig().defaultNextPage())) {
      if (candidateList->hasNext()) {
        candidateList->next();
        candidateList->toCursorMovable()->nextCandidate();
      } else if (candidateList->currentPage() > 0) {
        candidateList->setPage(0);
        candidateList->toCursorMovable()->nextCandidate();
      }
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Left)) ||
        key.check(fcitx::Key(FcitxKey_Page_Up)) ||
        key.check(fcitx::Key(FcitxKey_KP_Left)) ||
        key.check(fcitx::Key(FcitxKey_KP_Page_Up)) ||
        key.checkKeyList(instance_->globalConfig().defaultPrevPage())) {
      if (candidateList->hasPrev()) {
        candidateList->prev();
        candidateList->toCursorMovable()->nextCandidate();
      } else {
        int totalPages = candidateList->totalPages();
        if (totalPages > 0) {
          candidateList->setPage(totalPages - 1);
        }
        candidateList->toCursorMovable()->nextCandidate();
      }
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
  } else {
    if (key.check(fcitx::Key(FcitxKey_Right)) ||
        key.check(fcitx::Key(FcitxKey_KP_Right))) {
      candidateList->toCursorMovable()->nextCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Left)) ||
        key.check(fcitx::Key(FcitxKey_KP_Left))) {
      candidateList->toCursorMovable()->prevCandidate();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Down)) ||
        key.check(fcitx::Key(FcitxKey_KP_Down)) ||
        key.check(fcitx::Key(FcitxKey_Page_Down)) ||
        key.check(fcitx::Key(FcitxKey_KP_Page_Down)) ||
        key.checkKeyList(instance_->globalConfig().defaultNextPage())) {
      if (candidateList->hasNext()) {
        candidateList->next();
        candidateList->toCursorMovable()->nextCandidate();
      } else if (candidateList->currentPage() > 0) {
        candidateList->setPage(0);
        candidateList->toCursorMovable()->nextCandidate();
      }
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
    if (key.check(fcitx::Key(FcitxKey_Up)) ||
        key.check(fcitx::Key(FcitxKey_KP_Up)) ||
        key.check(fcitx::Key(FcitxKey_Page_Up)) ||
        key.check(fcitx::Key(FcitxKey_KP_Page_Up)) ||
        key.checkKeyList(instance_->globalConfig().defaultPrevPage())) {
      if (candidateList->hasPrev()) {
        candidateList->prev();
        candidateList->toCursorMovable()->nextCandidate();
      } else {
        int totalPages = candidateList->totalPages();
        if (totalPages > 0) {
          candidateList->setPage(totalPages - 1);
        }
        candidateList->toCursorMovable()->nextCandidate();
      }
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return true;
    }
  }

  if (associatedPhrases != nullptr) {
    if (!origKey.isModifier()) {
      return false;
    }
  }

  if (associatedPhrasesPlain != nullptr) {
    if (!origKey.isModifier()) {
      std::unique_ptr<InputStates::Empty> empty =
          std::make_unique<InputStates::Empty>();
      stateCallback(std::move(empty));
      return false;
    }
  }

  if (dynamic_cast<InputStates::ChoosingCandidate*>(state_.get()) != nullptr) {
    bool result =
        keyHandler_->handleCandidateKeyForTraditionalBopomofoIfRequired(
            MapFcitxKey(key, origKey),
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
      return true;
    }
  }

  // TODO(unassigned): All else... beep?
  errorCallback();
  return true;
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
  } else if (auto* associatePhrases =
                 dynamic_cast<InputStates::AssociatedPhrases*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, associatePhrases);
  } else if (auto* associatePhrasePlain =
                 dynamic_cast<InputStates::AssociatedPhrasesPlain*>(
                     currentPtr)) {
    handleCandidatesState(context, prevPtr, associatePhrasePlain);
  } else if (auto* selectingFeature =
                 dynamic_cast<InputStates::SelectingFeature*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, selectingFeature);
  } else if (auto* selectingDateMacro =
                 dynamic_cast<InputStates::SelectingDateMacro*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, selectingDateMacro);
  } else if (auto* chineseNumber =
                 dynamic_cast<InputStates::ChineseNumber*>(currentPtr)) {
    handleChineseNumberState(context, prevPtr, chineseNumber);
  } else if (auto* enclosingNumber =
                 dynamic_cast<InputStates::EnclosingNumber*>(currentPtr)) {
    handleEnclosingNumberState(context, prevPtr, enclosingNumber);
  } else if (auto* customMenu =
                 dynamic_cast<InputStates::CustomMenu*>(currentPtr)) {
    handleCandidatesState(context, prevPtr, customMenu);
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
                                             InputState* current) {
  std::unique_ptr<fcitx::CommonCandidateList> candidateList =
      std::make_unique<fcitx::CommonCandidateList>();

  auto keysConfig = config_.selectionKeys.value();
  selectionKeys_.clear();

  InputStates::AssociatedPhrases* associatedPhrases =
      dynamic_cast<InputStates::AssociatedPhrases*>(state_.get());
  InputStates::AssociatedPhrasesPlain* associatedPhrasesPlain =
      dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state_.get());
  bool useShiftKey =
      associatedPhrasesPlain != nullptr ||
      (associatedPhrases != nullptr && associatedPhrases->useShiftKey);

  if (useShiftKey) {
    // This is for label appearance only. Shift+[1-9] keys can only be checked
    // via a raw key's key code, but Keys constructed with "Shift-" names does
    // not carry proper key codes.
    selectionKeys_ = fcitx::Key::keyListFromString(
        "Shift+1 Shift+2 Shift+3 Shift+4 Shift+5 Shift+6 Shift+7 Shift+8 "
        "Shift+9");
#ifdef USE_LEGACY_FCITX5_API
#else
    std::vector<std::string> labels = {
        "⇧1. ", "⇧2. ", "⇧3. ", "⇧4. ", "⇧5. ", "⇧6. ", "⇧7. ", "⇧8. ", "⇧9. ",
    };
    candidateList->setLabels(labels);
#endif
    candidateList->setPageSize(static_cast<int>(selectionKeys_.size()));
  } else {
    if (keysConfig == SelectionKeys::Key_asdfghjkl) {
      selectionKeys_ = fcitx::Key::keyListFromString("a s d f g h j k l");
      numpadSelectionKeys_ = fcitx::KeyList();
    } else if (keysConfig == SelectionKeys::Key_asdfzxcvb) {
      selectionKeys_ = fcitx::Key::keyListFromString("a s d f z x c v b");
      numpadSelectionKeys_ = fcitx::KeyList();
    } else {
      selectionKeys_ = fcitx::Key::keyListFromString("1 2 3 4 5 6 7 8 9");
      numpadSelectionKeys_ = fcitx::KeyList();
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_1);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_2);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_3);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_4);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_5);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_6);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_7);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_8);
      numpadSelectionKeys_.emplace_back(FcitxKey_KP_9);
    }
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setPageSize(static_cast<int>(selectionKeys_.size()));
  }

  fcitx::CandidateLayoutHint layoutHint = getCandidateLayoutHint();
  candidateList->setLayoutHint(layoutHint);

  KeyHandler::StateCallback callback =
      [this, context](std::unique_ptr<InputState> next) {
        enterNewState(context, std::move(next));
      };

  auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(current);
  auto* selectingDictionary =
      dynamic_cast<InputStates::SelectingDictionary*>(current);
  auto* showingCharInfo = dynamic_cast<InputStates::ShowingCharInfo*>(current);
  auto* selectingFeature =
      dynamic_cast<InputStates::SelectingFeature*>(current);
  auto* selectingDateMacro =
      dynamic_cast<InputStates::SelectingDateMacro*>(current);
  auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(current);

  if (choosing != nullptr) {
    // Construct the candidate list with special care for candidates that have
    // the same values. The display text of such a candidate will be in the
    // form of "value (reading)" to help user disambiguate those candidates.

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
          fcitx::Text(displayText), c, choosing->originalCursor, keyHandler_,
          callback);
      // ownership of candidate is transferred to candidateList.
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoCandidateWord>(fcitx::Text(displayText), c,
                                                    choosing->originalCursor,
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
    menu.emplace_back(fmt::format(FmtRuntime(_("UTF8 String Length: {0}")),
                                  showingCharInfo->selectedPhrase.length()));
    size_t count = CodePointCount(showingCharInfo->selectedPhrase);
    menu.emplace_back(
        fmt::format(FmtRuntime(_("Code Point Count: {0}")), count));

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
  } else if (associatedPhrases != nullptr) {
    std::vector<InputStates::ChoosingCandidate::Candidate> candidates =
        associatedPhrases->candidates;

    for (const auto& c : candidates) {
#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate =
          new McBopomofoAssociatedPhraseCandidateWord(
              fcitx::Text(c.value), c, associatedPhrases->prefixReading,
              associatedPhrases->prefixValue,
              associatedPhrases->prefixCursorIndex, keyHandler_, callback);
      // ownership of candidate is transferred to candidateList.
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoAssociatedPhraseCandidateWord>(
              fcitx::Text(c.value), c, associatedPhrases->prefixReading,
              associatedPhrases->prefixValue,
              associatedPhrases->prefixCursorIndex, keyHandler_, callback);
      candidateList->append(std::move(candidate));
#endif
    }
  } else if (associatedPhrasesPlain != nullptr) {
    std::vector<InputStates::ChoosingCandidate::Candidate> candidates =
        associatedPhrasesPlain->candidates;
    for (const auto& c : candidates) {
#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate = new McBopomofoCandidateWord(
          fcitx::Text(c.value), c, 0, keyHandler_, callback);
      // ownership of candidate is transferred to candidateList.
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoCandidateWord>(fcitx::Text(c.value), c, 0,
                                                    keyHandler_, callback);
      candidateList->append(std::move(candidate));
#endif
    }
  } else if (selectingFeature != nullptr) {
    size_t index = 0;
    for (const auto& menuItem : selectingFeature->features) {
      std::string displayText = menuItem.name;

#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate =
          new McBopomofoFeatureWord(fcitx::Text(displayText), index,
                                    selectingFeature, keyHandler_, callback);
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoFeatureWord>(fcitx::Text(displayText),
                                                  index, selectingFeature,
                                                  keyHandler_, callback);
      candidateList->append(std::move(candidate));
#endif
      index++;
    }
  } else if (selectingDateMacro != nullptr) {
    for (const auto& displayText : selectingDateMacro->menu) {
#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate = new McBopomofoDirectInsertWord(
          fcitx::Text(displayText), displayText, callback);
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoDirectInsertWord>(fcitx::Text(displayText),
                                                       displayText, callback);
      candidateList->append(std::move(candidate));
#endif
    }
  } else if (customMenu != nullptr) {
    size_t index = 0;
    for (const auto& entry : customMenu->entries) {
      std::string displayText = entry.name;
#ifdef USE_LEGACY_FCITX5_API
      fcitx::CandidateWord* candidate = new McBopomofoCustomMenuWord(
          fcitx::Text(displayText), index, customMenu);
      candidateList->append(candidate);
#else
      std::unique_ptr<fcitx::CandidateWord> candidate =
          std::make_unique<McBopomofoCustomMenuWord>(fcitx::Text(displayText),
                                                     index, customMenu);
      candidateList->append(std::move(candidate));
#endif
      index++;
    }
  }

  candidateList->toCursorMovable()->nextCandidate();
  context->inputPanel().reset();
  context->inputPanel().setCandidateList(std::move(candidateList));
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

  auto* notEmpty = dynamic_cast<InputStates::NotEmpty*>(current);
  if (notEmpty != nullptr) {
    updatePreedit(context, notEmpty);
  }
}

void McBopomofoEngine::handleMarkingState(fcitx::InputContext* context,
                                          InputState* /*unused*/,
                                          InputStates::Marking* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  updatePreedit(context, current);
}

void McBopomofoEngine::handleChineseNumberState(
    fcitx::InputContext* context, InputState* /*unused*/,
    InputStates::ChineseNumber* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

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
  preedit.append(current->composingBuffer(), normalFormat);
  preedit.setCursor(static_cast<int>(current->composingBuffer().length()));

  if (useClientPreedit) {
    context->inputPanel().setClientPreedit(preedit);
  } else {
    context->inputPanel().setPreedit(preedit);
  }
  context->updatePreedit();
}

void McBopomofoEngine::handleEnclosingNumberState(
    fcitx::InputContext* context, InputState* /*unused*/,
    InputStates::EnclosingNumber* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

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
  preedit.append(current->composingBuffer(), normalFormat);
  preedit.setCursor(static_cast<int>(current->composingBuffer().length()));

  if (useClientPreedit) {
    context->inputPanel().setClientPreedit(preedit);
  } else {
    context->inputPanel().setPreedit(preedit);
  }
  context->updatePreedit();
}

fcitx::CandidateLayoutHint McBopomofoEngine::getCandidateLayoutHint() const {
  fcitx::CandidateLayoutHint layoutHint = fcitx::CandidateLayoutHint::NotSet;

  if (dynamic_cast<InputStates::SelectingDictionary*>(state_.get()) !=
          nullptr ||
      dynamic_cast<InputStates::ShowingCharInfo*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::SelectingFeature*>(state_.get()) != nullptr ||
      dynamic_cast<InputStates::SelectingDateMacro*>(state_.get()) != nullptr) {
    return fcitx::CandidateLayoutHint::Vertical;
  }

  auto* choosingCandidate =
      dynamic_cast<InputStates::ChoosingCandidate*>(state_.get());
  if (choosingCandidate != nullptr) {
    auto candidates = choosingCandidate->candidates;
    for (const InputStates::ChoosingCandidate::Candidate& candidate :
         candidates) {
      std::string value = candidate.value;
      if (McBopomofo::CodePointCount(value) >
          kForceVerticalCandidateThreshold) {
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

void McBopomofoEngine::showAndClearUserFileIssues() {
  size_t numIssues = 0;
  const size_t MAX_ISSUES = 3;

  std::stringstream sst;
  for (const auto& issue : userFileIssues_) {
    switch (issue.fileType) {
      case McBopomofoLM::UserFileType::USER_PHRASES:
        sst << _("User Phrases");
        break;
      case McBopomofoLM::UserFileType::EXCLUDED_PHRASES:
        sst << _("Excluded Phrases");
        break;
      case McBopomofoLM::UserFileType::PHRASE_REPLACEMENT_MAP:
        sst << _("Phrase Replacement File");
        break;
      default:
        // should not happen
        break;
    }

    sst << " (";
    sst << issue.path.string();
    sst << ") ";
    sst << fmt::format(FmtRuntime(_("line {0}")), issue.lineNumber);
    sst << ": ";

    switch (issue.issueType) {
      case McBopomofoLM::IssueType::NO_ISSUE:
        // Ignored.
        break;
      case McBopomofoLM::IssueType::MISSING_SECOND_COLUMN:
        sst << _("Only one column was found.");
        break;
      case McBopomofoLM::IssueType::NULL_CHARACTER_IN_TEXT:
        sst << _("Illegal NULL character was found.");
        break;
      default:
        // should not happen
        break;
    }
    sst << "\n";

    ++numIssues;
    if (numIssues >= MAX_ISSUES) {
      break;
    }
  }

  if (userFileIssues_.size() > MAX_ISSUES) {
    sst << "\n";
    size_t remainingIssues = userFileIssues_.size() - MAX_ISSUES;

    if (remainingIssues == 1) {
      sst << _("And one more issue.");
    } else {
      sst << fmt::format(FmtRuntime(_("And {0} more issues.")),
                         remainingIssues);
    }
    sst << "\n";
  }

  if (notifications()) {
    notifications()->call<fcitx::INotifications::showTip>(
        "mcbopomofo-user-file-issues", _("McBopomofo"), "fcitx_mcbopomofo",
        _("Issues found in user files"), sst.str(),
        kFcitx5NotificationTimeoutInMs);
  } else {
    FCITX_MCBOPOMOFO_ERROR() << sst.str();
  }
  userFileIssues_.clear();
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

}  // namespace McBopomofo
