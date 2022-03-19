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

#include "ParselessLM.h"

namespace McBopomofo {

// TODO(unassigned): Remove this after everything is implemented.
#pragma GCC diagnostic ignored "-Wunused-parameter"

static const char* kDataPath = "data/mcbopomofo-data.txt";
static const char* kConfigPath = "conf/mcbopomofo.conf";

// TODO(unassigned): Remove this once fcitx::DisplayOnlyCandidateWord is
// available through Ubuntu.
class DisplayOnlyCandidateWord : public fcitx::CandidateWord {
 public:
  explicit DisplayOnlyCandidateWord(fcitx::Text text)
      : fcitx::CandidateWord(text) {}
  void select(fcitx::InputContext* inputContext) const override {}
};

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
  keyHandler_->setConvertToSimplifiedChinese(
      config_.convertsToSimplifiedChinese.value());
  keyHandler_->setMapDvorakToQwerty(config_.mapsDvorakToQwerty.value());

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
  fooBuffer_ = "";
  enterNewState(event.inputContext(), std::make_unique<InputStates::Empty>());
}

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry&,
                                fcitx::KeyEvent& keyEvent) {
  FCITX_INFO() << keyEvent.key() << " isRelease=" << keyEvent.isRelease()
               << " isInputContextEvent=" << keyEvent.isInputContextEvent();

  if (!keyEvent.isInputContextEvent()) {
    return;
  }

  if (keyEvent.isRelease()) {
    return;
  }

  fcitx::InputContext* context = keyEvent.inputContext();

  // If candidate list is on.
  auto current_candidate_list = context->inputPanel().candidateList();
  if (current_candidate_list != nullptr) {
    int idx = keyEvent.key().keyListIndex(selectionKeys_);
    if (idx >= 0) {
      if (idx < current_candidate_list->size()) {
        keyEvent.filterAndAccept();
        auto selectedCandidate = current_candidate_list->candidate(idx);
        auto committing = std::make_unique<InputStates::Committing>();
        committing->setPoppedText(selectedCandidate->text().toString());
        enterNewState(context, std::move(committing));
        fooBuffer_ = "";
        return;
      }
    }

    if (keyEvent.key().check(FcitxKey_Escape)) {
      keyEvent.filterAndAccept();

      auto inputting = std::make_unique<InputStates::Inputting>();
      inputting->setComposingBuffer(fooBuffer_);
      inputting->setCursorIndex(fooBuffer_.size());
      enterNewState(context, std::move(inputting));
      return;
    }

    // Absorb all other keys.
    keyEvent.filterAndAccept();
    return;
  }

  if (keyEvent.key().check(FcitxKey_Return) ||
      keyEvent.key().check(FcitxKey_KP_Enter)) {
    if (fooBuffer_.empty()) {
      return;
    }

    auto committing = std::make_unique<InputStates::Committing>();
    committing->setPoppedText(fooBuffer_);
    enterNewState(context, std::move(committing));
    fooBuffer_ = "";
    return;
  }

  if (keyEvent.key().check(FcitxKey_space)) {
    if (fooBuffer_.empty()) {
      return;
    }

    keyEvent.filterAndAccept();

    auto choosingCandidate = std::make_unique<InputStates::ChoosingCandidate>();
    choosingCandidate->setCandidates({"hello", "world", "foobar", "baz"});
    choosingCandidate->setComposingBuffer(fooBuffer_);
    choosingCandidate->setCursorIndex(fooBuffer_.size());
    enterNewState(context, std::move(choosingCandidate));
    return;
  }

  if (keyEvent.key().check(FcitxKey_Escape)) {
    if (fooBuffer_.empty()) {
      return;
    }

    fooBuffer_.clear();
    keyEvent.filterAndAccept();
    enterNewState(context,
                  std::make_unique<InputStates::EmptyIgnoringPrevious>());
    return;
  }

  if (keyEvent.key().check(FcitxKey_BackSpace)) {
    if (fooBuffer_.empty()) {
      return;
    }

    keyEvent.filterAndAccept();
    fooBuffer_ = fooBuffer_.substr(0, fooBuffer_.length() - 1);
    if (fooBuffer_.empty()) {
      enterNewState(context,
                    std::make_unique<InputStates::EmptyIgnoringPrevious>());
    } else {
      auto inputting = std::make_unique<InputStates::Inputting>();
      inputting->setComposingBuffer(fooBuffer_);
      inputting->setCursorIndex(fooBuffer_.size());
      enterNewState(context, std::move(inputting));
    }
    return;
  }

  if (keyEvent.key().isLAZ() || keyEvent.key().isDigit()) {
    keyEvent.filterAndAccept();
    fooBuffer_.append(fcitx::Key::keySymToString(keyEvent.key().sym()));
    auto inputting = std::make_unique<InputStates::Inputting>();
    inputting->setComposingBuffer(fooBuffer_);
    inputting->setCursorIndex(fooBuffer_.size());
    enterNewState(context, std::move(inputting));
    return;
  }

  // Absorb other keys if buffer is not empty.
  if (!fooBuffer_.empty()) {
    keyEvent.filterAndAccept();
    return;
  }

  // Otherwise, fall through.
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
                                        InputState* prev,
                                        InputStates::Empty* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
  if (auto notEmpty = dynamic_cast<InputStates::NotEmpty*>(prev)) {
    context->commitString(notEmpty->composingBuffer());
  }
}

void McBopomofoEngine::handleEmptyIgnoringPreviousState(
    fcitx::InputContext* context, InputState* prev,
    InputStates::EmptyIgnoringPrevious* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
}

void McBopomofoEngine::handleCommittingState(fcitx::InputContext* context,
                                             InputState* prev,
                                             InputStates::Committing* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  context->updatePreedit();
  if (!current->poppedText().empty()) {
    context->commitString(current->poppedText());
  }
}

void McBopomofoEngine::handleInputtingState(fcitx::InputContext* context,
                                            InputState* prev,
                                            InputStates::Inputting* current) {
  context->inputPanel().reset();
  context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
  if (!current->poppedText().empty()) {
    context->commitString(current->poppedText());
  }
  updatePreedit(context, current);
}

void McBopomofoEngine::handleCandidatesState(
    fcitx::InputContext* context, InputState* prev,
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
    // TODO(unassigned): Migrate to the new fcitx5 API using the commented-out
    // code below once on the latest API.
    // std::unique_ptr<fcitx::CandidateWord> candidate =
    // std::make_unique<fcitx::CandidateWord>(fcitx::Text(candidateStr));

    fcitx::CandidateWord* candidate =
        new DisplayOnlyCandidateWord(fcitx::Text(candidateStr));
    // ownership of candidate is transferred to candidateList.
    candidateList->append(candidate);
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
                                    : fcitx::TextFormatFlag::None};

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

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

#pragma GCC diagnostic pop

}  // namespace McBopomofo
