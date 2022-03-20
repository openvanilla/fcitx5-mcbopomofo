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
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "ChineseConverter.h"
#include "ParselessLM.h"

namespace McBopomofo {

static const char* kDataPath = "data/mcbopomofo-data.txt";
static const char* kConfigPath = "conf/mcbopomofo.conf";

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
  }
  return c;
}

static fcitx::Key ConvertDvorakToQwerty(fcitx::Key key) {
  if (!key.isSimple()) {
    return key;
  }

  // For simple keys, it's guaranteed to be downcastable.
  char sym = CovertDvorakToQwerty(key.sym());
  return fcitx::Key(static_cast<fcitx::KeySym>(sym));
}

class EmptyLM : public Formosa::Gramambular::LanguageModel {
 public:
  const std::vector<Formosa::Gramambular::Bigram> bigramsForKeys(
      const std::string&, const std::string&) override {
    return std::vector<Formosa::Gramambular::Bigram>();
  }
  const std::vector<Formosa::Gramambular::Unigram> unigramsForKey(
      const std::string&) override {
    return std::vector<Formosa::Gramambular::Unigram>();
  }
  bool hasUnigramsForKey(const std::string&) override { return false; }
};

McBopomofoEngine::McBopomofoEngine() {
  std::string path;
  path = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kDataPath);

  std::shared_ptr<Formosa::Gramambular::LanguageModel> lm =
      std::make_shared<EmptyLM>();
  if (std::filesystem::exists(path)) {
    FCITX_INFO() << "found McBopomofo data: " << path;

    std::shared_ptr<ParselessLM> parseless_lm = std::make_shared<ParselessLM>();
    bool result = parseless_lm->open(path);
    if (result) {
      FCITX_INFO() << "language model successfully opened";
      lm = std::move(parseless_lm);
    }
  }

  keyHandler_ = std::make_unique<KeyHandler>(std::move(lm));
  state_ = std::make_unique<InputStates::Empty>();

  // Required by convention of fcitx5 modules to load config on its own.
  reloadConfig();
}

const fcitx::Configuration* McBopomofoEngine::getConfig() const {
  FCITX_INFO() << "getConfig";
  return &config_;
}

void McBopomofoEngine::setConfig(const fcitx::RawConfig& config) {
  FCITX_INFO() << "setConfig";
  config_.load(config, true);
  fcitx::safeSaveAsIni(config_, kConfigPath);
}

void McBopomofoEngine::reloadConfig() {
  FCITX_INFO() << "reloadConfig";
  fcitx::readAsIni(config_, kConfigPath);
}

void McBopomofoEngine::activate(const fcitx::InputMethodEntry&,
                                fcitx::InputContextEvent&) {
  auto layout = Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
  switch (config_.bopomofoKeyboardLayout.value()) {
    case BopomofoKeyboardLayout::Standard:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
      break;
    case BopomofoKeyboardLayout::Eten:
      layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETenLayout();
      break;
  }
  keyHandler_->setKeyboardLayout(layout);
}

void McBopomofoEngine::reset(const fcitx::InputMethodEntry&,
                             fcitx::InputContextEvent& event) {
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

    fcitx::CommonCandidateList* maybeCandidateList =
        dynamic_cast<fcitx::CommonCandidateList*>(
            context->inputPanel().candidateList().get());
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
      std::string candidate = candidateList->candidate(idx).text().toString();
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

    // Optimization: set the current state to empty.
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
  }
}
void McBopomofoEngine::handleEmptyState(fcitx::InputContext* context,
                                        InputState* prev, InputStates::Empty*) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
  if (auto notEmpty = dynamic_cast<InputStates::NotEmpty*>(prev)) {
    commitString(context, notEmpty->composingBuffer());
  }
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
  context->updatePreedit();
  if (!current->poppedText().empty()) {
    commitString(context, current->poppedText());
  }
}

void McBopomofoEngine::handleInputtingState(fcitx::InputContext* context,
                                            InputState*,
                                            InputStates::Inputting* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (!current->poppedText().empty()) {
    commitString(context, current->poppedText());
  }
  updatePreedit(context, current);
}

void McBopomofoEngine::handleCandidatesState(
    fcitx::InputContext* context, InputState*,
    InputStates::ChoosingCandidate* current) {
  std::unique_ptr<fcitx::CommonCandidateList> candidateList =
      std::make_unique<fcitx::CommonCandidateList>();

  // TODO(unassigned): Make this configurable; read from KeyHandler or some
  // config
  selectionKeys_.clear();
  selectionKeys_.emplace_back(FcitxKey_1);
  selectionKeys_.emplace_back(FcitxKey_2);
  selectionKeys_.emplace_back(FcitxKey_3);
  selectionKeys_.emplace_back(FcitxKey_4);
  selectionKeys_.emplace_back(FcitxKey_5);
  selectionKeys_.emplace_back(FcitxKey_6);
  selectionKeys_.emplace_back(FcitxKey_7);
  selectionKeys_.emplace_back(FcitxKey_8);
  selectionKeys_.emplace_back(FcitxKey_9);

  candidateList->setSelectionKey(selectionKeys_);
  candidateList->setPageSize(selectionKeys_.size());

  for (const std::string& candidateStr : current->candidates()) {
    std::unique_ptr<fcitx::CandidateWord> candidate =
        std::make_unique<fcitx::DisplayOnlyCandidateWord>(
            fcitx::Text(candidateStr));
    candidateList->append(std::move(candidate));
  }
  context->inputPanel().setCandidateList(std::move(candidateList));
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);

  updatePreedit(context, current);
}
void McBopomofoEngine::updatePreedit(fcitx::InputContext* context,
                                     InputStates::NotEmpty* state) {
  bool use_client_preedit =
      context->capabilityFlags().test(fcitx::CapabilityFlag::Preedit);
  fcitx::TextFormatFlags format{use_client_preedit
                                    ? fcitx::TextFormatFlag::Underline
                                    : fcitx::TextFormatFlag::NoFlag};

  fcitx::Text preedit;
  preedit.append(state->composingBuffer(), format);
  preedit.setCursor(state->cursorIndex());
  if (use_client_preedit) {
    context->inputPanel().setClientPreedit(preedit);
  } else {
    context->inputPanel().setPreedit(preedit);
  }
  context->updatePreedit();
}

void McBopomofoEngine::commitString(fcitx::InputContext* context,
                                    std::string text) {
  if (config_.convertsToSimplifiedChinese.value()) {
    context->commitString(ConvertToSimplifiedChinese(text));
  } else {
    context->commitString(text);
  }
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

}  // namespace McBopomofo
