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

#include <fcitx-utils/standardpath.h>
#include <fcitx/candidatelist.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/userinterfacemanager.h>

#include <memory>
#include <utility>
#include <vector>

namespace McBopomofo {

constexpr char kConfigPath[] = "conf/mcbopomofo.conf";

static char CovertDvorakToQwerty(char c) {
  switch (c) {
    case '[':
      return '-';
    case ']':
      return '=';
    case '{':
      return '_';
    case '}':
      return '+';
    case '\'':
      return 'q';
    case ',':
      return 'w';
    case '.':
      return 'e';
    case 'p':
      return 'r';
    case 'y':
      return 't';
    case 'f':
      return 'y';
    case 'g':
      return 'u';
    case 'c':
      return 'i';
    case 'r':
      return 'o';
    case 'l':
      return 'p';
    case '/':
      return '[';
    case '=':
      return ']';
    case '?':
      return '{';
    case '+':
      return '}';
    case 'o':
      return 's';
    case 'e':
      return 'd';
    case 'u':
      return 'f';
    case 'i':
      return 'g';
    case 'd':
      return 'h';
    case 'h':
      return 'j';
    case 't':
      return 'k';
    case 'n':
      return 'l';
    case 's':
      return ';';
    case '-':
      return '\'';
    case 'S':
      return ':';
    case '_':
      return '"';
    case ';':
      return 'z';
    case 'q':
      return 'x';
    case 'j':
      return 'c';
    case 'k':
      return 'v';
    case 'x':
      return 'b';
    case 'b':
      return 'n';
    case 'w':
      return ',';
    case 'v':
      return '.';
    case 'z':
      return '/';
    case 'W':
      return '<';
    case 'V':
      return '>';
    case 'Z':
      return '?';
    default:
      return c;
  }
}

static fcitx::Key ConvertDvorakToQwerty(fcitx::Key key) {
  if (!key.isSimple()) {
    return key;
  }

  // For simple keys, it's guaranteed to be downcastable.
  char sym = CovertDvorakToQwerty(key.sym());
  return fcitx::Key(static_cast<fcitx::KeySym>(sym));
}

#ifdef USE_LEGACY_FCITX5_API
class DisplayOnlyCandidateWord : public fcitx::CandidateWord {
 public:
  explicit DisplayOnlyCandidateWord(fcitx::Text text)
      : fcitx::CandidateWord(std::move(text)) {}
  void select(fcitx::InputContext*) const override {}
};
#endif

McBopomofoEngine::McBopomofoEngine(fcitx::Instance* instance)
    : instance_(instance) {
  languageModelLoader_ = std::make_shared<LanguageModelLoader>();
  keyHandler_ = std::make_unique<KeyHandler>(languageModelLoader_->getLM(),
                                             languageModelLoader_);
  state_ = std::make_unique<InputStates::Empty>();

  editUserPhreasesAction_ = std::make_unique<fcitx::SimpleAction>();
  editUserPhreasesAction_->setShortText(_("Edit User Phrases"));
  editUserPhreasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        auto command = "xdg-open " + languageModelLoader_->userPhrasesPath();
        system(command.c_str());
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-phrases-edit", editUserPhreasesAction_.get());

  excludedPhreasesAction_ = std::make_unique<fcitx::SimpleAction>();
  excludedPhreasesAction_->setShortText(_("Edit Excluded Phrases"));
  excludedPhreasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        auto command =
            "xdg-open " + languageModelLoader_->excludedPhrasesPath();
        system(command.c_str());
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-excluded-phrases-edit", excludedPhreasesAction_.get());

  reloadUserPhreasesAction_ = std::make_unique<fcitx::SimpleAction>();
  reloadUserPhreasesAction_->setShortText(_("Reload User Phrases"));
  reloadUserPhreasesAction_->connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext*) {
        languageModelLoader_->reloadUserModelsIfNeeded();
      });
  instance_->userInterfaceManager().registerAction(
      "mcbopomofo-user-phrases-reload", reloadUserPhreasesAction_.get());

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
  inputContext->statusArea().addAction(fcitx::StatusGroup::InputMethod,
                                       reloadUserPhreasesAction_.get());

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

  languageModelLoader_->reloadUserModelsIfNeeded();
}

void McBopomofoEngine::reset(const fcitx::InputMethodEntry&,
                             fcitx::InputContextEvent& event) {
  auto isFocusOutEvent = dynamic_cast<fcitx::FocusOutEvent*>(&event) != nullptr;

  // Note: when inputting in Chrome based web browsers, the `reset` method would
  // be called when the input method commits evicted text by passing a focus out
  // event. We have to ignore such events otherwise we are hardly to input a
  // complete Chinese sentense.
  if (isFocusOutEvent) {
    auto context = event.inputContext();
    auto program = context->program();
    if (program.rfind("google-chrome", 0) == 0) return;
    if (program.rfind("microsoft-edge", 0) == 0) return;
    if (program.rfind("sidekick-browser", 0) == 0) return;
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
  if (config_.mapsDvorakToQwerty.value()) {
    key = ConvertDvorakToQwerty(key);
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
      FCITX_WARN() << "inconsistent state";
      enterNewState(context, std::make_unique<InputStates::Empty>());
      return;
    }

    handleCandidateKeyEvent(context, key, maybeCandidateList);
    return;
  }

  bool accepted = keyHandler_->handle(
      key, state_.get(),
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

  if (key.check(FcitxKey_Escape)) {
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
    } else if (candidateList->currentPage() > 0) {
      candidateList->setPage(0);
    }
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  static std::array<fcitx::Key, 3> nextKeys{fcitx::Key(FcitxKey_Page_Down),
                                            fcitx::Key(FcitxKey_Right),
                                            fcitx::Key(FcitxKey_Down)};
  static std::array<fcitx::Key, 3> prevKeys{fcitx::Key(FcitxKey_Page_Up),
                                            fcitx::Key(FcitxKey_Left),
                                            fcitx::Key(FcitxKey_Up)};

  if (key.checkKeyList(nextKeys) && candidateList->hasNext()) {
    candidateList->next();
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  if (key.checkKeyList(prevKeys) && candidateList->hasPrev()) {
    candidateList->prev();
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  // TODO(unassigned): All else... beep?
}

void McBopomofoEngine::enterNewState(fcitx::InputContext* context,
                                     std::unique_ptr<InputState> newState) {
  // Hold the previous state, and transfer the ownership of newState.
  std::unique_ptr<InputState> prevState = std::move(state_);
  state_ = std::move(newState);

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
    fcitx::CandidateWord* candidate =
        new DisplayOnlyCandidateWord(fcitx::Text(candidateStr));
    // ownership of candidate is transferred to candidateList.
    candidateList->append(candidate);
#else
    std::unique_ptr<fcitx::CandidateWord> candidate =
        std::make_unique<fcitx::DisplayOnlyCandidateWord>(
            fcitx::Text(candidateStr));
    candidateList->append(std::move(candidate));
#endif
  }
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
  bool use_client_preedit =
      context->capabilityFlags().test(fcitx::CapabilityFlag::Preedit);
#ifdef USE_LEGACY_FCITX5_API
  fcitx::TextFormatFlags normalFormat{use_client_preedit
                                          ? fcitx::TextFormatFlag::Underline
                                          : fcitx::TextFormatFlag::None};
#else
  fcitx::TextFormatFlags normalFormat{use_client_preedit
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

  if (use_client_preedit) {
    context->inputPanel().setClientPreedit(preedit);
  } else {
    context->inputPanel().setPreedit(preedit);
  }

  context->inputPanel().setAuxDown(fcitx::Text(state->tooltip));
  context->updatePreedit();
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

}  // namespace McBopomofo
