#include "McBopomofo.h"
#include "ParselessLM.h"
#include <fcitx-utils/standardpath.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <filesystem>
#include <memory>

// TODO: Remove this after everything is implemented.
#pragma GCC diagnostic ignored "-Wunused-parameter"

static const char* kDataPath = "data/mcbopomofo-data.txt";
static const char* kConfigPath = "conf/mcbopomofo.conf";

// TODO: Remove this once fcitx::DisplayOnlyCandidateWord is available through
// Ubuntu.
class DisplayOnlyCandidateWord : public fcitx::CandidateWord {
public:
  DisplayOnlyCandidateWord(fcitx::Text text) : fcitx::CandidateWord(text) {}
  void select(fcitx::InputContext* inputContext) const override {}
};

McBopomofoEngine::McBopomofoEngine() {
  std::string path;
  path = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kDataPath);

  if (std::filesystem::exists(path)) {
    FCITX_INFO() << "found McBopomofo data: " << path;

    McBopomofo::ParselessLM lm;
    lm.open(path);
    auto unigrams = lm.unigramsForKey("ㄒㄧㄠˇ");
    for (const auto& unigram : unigrams) {
      FCITX_INFO() << unigram;
    }
    lm.close();
  }

  m_state = new McBopomofo::InputStateEmpty();

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

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry& entry,
                                fcitx::KeyEvent& keyEvent) {
  FCITX_UNUSED(entry);

  if (config_.bopomofoKeyboardLayout.value() == BopomofoKeyboardLayout::Eten) {
    FCITX_INFO() << "using eten layout";
  } else {
    FCITX_INFO() << "using standard layout";
  }

  FCITX_INFO() << "convert Dvorak to QWERTY: "
               << config_.debugMapDvorakBackToQwerty.value();
  FCITX_INFO() << keyEvent.key() << " isRelease=" << keyEvent.isRelease();

  if (!keyEvent.isInputContextEvent()) {
    return;
  }

  if (keyEvent.isRelease()) {
    return;
  }

  fcitx::InputContext* context = keyEvent.inputContext();
  const bool use_client_preedit =
      context->capabilityFlags().test(fcitx::CapabilityFlag::Preedit);
  fcitx::TextFormatFlags format{use_client_preedit
                                    ? fcitx::TextFormatFlag::Underline
                                    : fcitx::TextFormatFlag::None};

  // If candidate list is on.
  auto current_candidate_list = context->inputPanel().candidateList();
  if (current_candidate_list != nullptr) {
    int idx = keyEvent.key().keyListIndex(selection_keys_);
    if (idx >= 0) {
      if (idx < current_candidate_list->size()) {
        keyEvent.accept();

        auto selectedCandidate = current_candidate_list->candidate(idx);
        context->inputPanel().reset();
        context->updatePreedit();
        context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        context->commitString(selectedCandidate->text().toString());
        foo_buffer_ = "";
        return;
      }
    }

    if (keyEvent.key().check(FcitxKey_Escape)) {
      keyEvent.accept();

      // Reset the panel, then update the preedit again.
      context->inputPanel().reset();

      fcitx::Text preedit;
      preedit.append(foo_buffer_, format);
      preedit.setCursor(foo_buffer_.length());
      if (use_client_preedit) {
        context->inputPanel().setClientPreedit(preedit);
      } else {
        context->inputPanel().setPreedit(preedit);
      }
      context->updatePreedit();
      context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
      return;
    }

    // Absorb all other keys.
    keyEvent.accept();
    return;
  }

  if (keyEvent.key().check(FcitxKey_Return) ||
      keyEvent.key().check(FcitxKey_KP_Enter)) {
    if (foo_buffer_.empty()) {
      return;
    }

    keyEvent.filterAndAccept();
    context->inputPanel().reset();
    context->updatePreedit();
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    context->commitString(foo_buffer_);
    foo_buffer_ = "";
    return;
  }

  if (keyEvent.key().check(FcitxKey_space)) {
    if (foo_buffer_.empty()) {
      return;
    }

    keyEvent.filterAndAccept();

    std::unique_ptr<fcitx::CommonCandidateList> candidate_list =
        std::make_unique<fcitx::CommonCandidateList>();

    selection_keys_.clear();
    selection_keys_.emplace_back(FcitxKey_1);
    selection_keys_.emplace_back(FcitxKey_2);
    selection_keys_.emplace_back(FcitxKey_3);
    candidate_list->setSelectionKey(selection_keys_);
    candidate_list->setPageSize(3);

    // TODO: Migrate to the new fcitx5 API using the commented-out code below
    // once on the latest API.
    /*
    std::unique_ptr<fcitx::CandidateWord> candidate0 =
    std::make_unique<fcitx::CandidateWord>(fcitx::Text{"hello"});
    std::unique_ptr<fcitx::CandidateWord> candidate1 =
    std::make_unique<fcitx::CandidateWord>(fcitx::Text{"world"});
     */

    // The current impl uses shared_ptr and will take over ownership of these
    // created instances!
    fcitx::CandidateWord* candidate_0 =
        new DisplayOnlyCandidateWord(fcitx::Text{"hello"});
    fcitx::CandidateWord* candidate_1 =
        new DisplayOnlyCandidateWord(fcitx::Text{"world"});
    candidate_list->insert(0, candidate_0);
    candidate_list->insert(1, candidate_1);
    context->inputPanel().setCandidateList(std::move(candidate_list));
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  if (keyEvent.key().check(FcitxKey_Escape)) {
    if (foo_buffer_.empty()) {
      return;
    }

    foo_buffer_.clear();
    keyEvent.filterAndAccept();
    context->inputPanel().reset();
    context->updatePreedit();
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  if (keyEvent.key().check(FcitxKey_BackSpace)) {
    if (foo_buffer_.empty()) {
      return;
    }

    keyEvent.filterAndAccept();
    foo_buffer_ = foo_buffer_.substr(0, foo_buffer_.length() - 1);

    fcitx::Text preedit;
    preedit.append(foo_buffer_, format);
    preedit.setCursor(foo_buffer_.length());
    if (use_client_preedit) {
      context->inputPanel().setClientPreedit(preedit);
    } else {
      context->inputPanel().setPreedit(preedit);
    }
    context->updatePreedit();
    return;
  }

  if (keyEvent.key().isLAZ() || keyEvent.key().isDigit()) {
    keyEvent.filterAndAccept();
    foo_buffer_.append(fcitx::Key::keySymToString(keyEvent.key().sym()));
    fcitx::Text preedit;
    preedit.append(foo_buffer_, format);
    preedit.setCursor(foo_buffer_.length());
    if (use_client_preedit) {
      context->inputPanel().setClientPreedit(preedit);
    } else {
      context->inputPanel().setPreedit(preedit);
    }
    context->updatePreedit();
    context->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    return;
  }

  // Absorb other keys if buffer is not empty.
  if (!foo_buffer_.empty()) {
    keyEvent.filterAndAccept();
    return;
  }
}

void McBopomofoEngine::handle(McBopomofo::InputState* newState) {
  auto current = m_state;
  if (auto empty = dynamic_cast<McBopomofo::InputStateEmpty*>(newState)) {
    handleEmpty(empty, current);
  } else if (auto emptyIgnoringPrevious =
                 dynamic_cast<McBopomofo::InputStateEmptyIgnoringPrevious*>(
                     newState)) {
    handleEmptyIgnoringPrevious(emptyIgnoringPrevious, current);
  } else if (auto committing =
                 dynamic_cast<McBopomofo::InputStateCommitting*>(newState)) {
    handleCommitting(committing, current);
  } else if (auto inputting =
                 dynamic_cast<McBopomofo::InputStateInputting*>(newState)) {
    handleInputting(inputting, current);
  } else if (auto candidates =
                 dynamic_cast<McBopomofo::InputStateChoosingCandidate*>(
                     newState)) {
    handleCandidates(candidates, current);
  }
  m_state = newState;
}

void McBopomofoEngine::handleEmpty(McBopomofo::InputStateEmpty* newState,
                                   McBopomofo::InputState* state) {
  // implement this
}

void McBopomofoEngine::handleEmptyIgnoringPrevious(
    McBopomofo::InputStateEmptyIgnoringPrevious* newState,
    McBopomofo::InputState* state) {
  // implement this
}

void McBopomofoEngine::handleCommitting(
    McBopomofo::InputStateCommitting* newState, McBopomofo::InputState* state) {
  // implement this
}

void McBopomofoEngine::handleInputting(
    McBopomofo::InputStateInputting* newState, McBopomofo::InputState* state) {
  // implement this
}

void McBopomofoEngine::handleCandidates(
    McBopomofo::InputStateChoosingCandidate* newState,
    McBopomofo::InputState* state) {
  // implement this
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

#pragma GCC diagnostic pop
