#include "McBopomofo.h"

// TODO: Remove this after everything is implemented.
#pragma GCC diagnostic ignored "-Wunused-parameter"

McBopomofoEngine::McBopomofoEngine() {
  m_state = new McBopomofo::InputStateEmpty();
}

void McBopomofoEngine::keyEvent(const fcitx::InputMethodEntry &entry,
                                fcitx::KeyEvent &keyEvent) {
  FCITX_UNUSED(entry);
  FCITX_INFO() << keyEvent.key() << " isRelease=" << keyEvent.isRelease();
}

void McBopomofoEngine::handle(McBopomofo::InputState *newState) {
  auto current = m_state;
  if (auto empty = dynamic_cast<McBopomofo::InputStateEmpty *>(newState)) {
    handleEmpty(empty, current);
  } else if (auto emptyIgnoringPrevious =
                 dynamic_cast<McBopomofo::InputStateEmptyIgnoringPrevious *>(
                     newState)) {
    handleEmptyIgnoringPrevious(emptyIgnoringPrevious, current);
  } else if (auto committing =
                 dynamic_cast<McBopomofo::InputStateCommitting *>(newState)) {
    handleCommitting(committing, current);
  } else if (auto inputting =
                 dynamic_cast<McBopomofo::InputStateInputting *>(newState)) {
    handleInputting(inputting, current);
  } else if (auto candidates =
                 dynamic_cast<McBopomofo::InputStateChoosingCandidate *>(
                     newState)) {
    handleCandidates(candidates, current);
  }
  m_state = newState;
}

void McBopomofoEngine::handleEmpty(McBopomofo::InputStateEmpty *newState,
                                   McBopomofo::InputState *state) {
  // implement this
}

void McBopomofoEngine::handleEmptyIgnoringPrevious(
    McBopomofo::InputStateEmptyIgnoringPrevious *newState,
    McBopomofo::InputState *state) {
  // implement this
}

void McBopomofoEngine::handleCommitting(
    McBopomofo::InputStateCommitting *newState, McBopomofo::InputState *state) {
  // implement this
}

void McBopomofoEngine::handleInputting(
    McBopomofo::InputStateInputting *newState, McBopomofo::InputState *state) {
  // implement this
}

void McBopomofoEngine::handleCandidates(
    McBopomofo::InputStateChoosingCandidate *newState,
    McBopomofo::InputState *state) {
  // implement this
}

FCITX_ADDON_FACTORY(McBopomofoEngineFactory);

#pragma GCC diagnostic pop
