#ifndef _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
#define _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_

#include "InputState.h"
#include <fcitx/addonfactory.h>
#include <fcitx/inputmethodengine.h>

class McBopomofoEngine : public fcitx::InputMethodEngine {

public:
  McBopomofoEngine();
  void keyEvent(const fcitx::InputMethodEntry &entry,
                fcitx::KeyEvent &keyEvent) override;

private:
  McBopomofo::InputState *m_state;
  void handle(McBopomofo::InputState *newState);
  void handleEmpty(McBopomofo::InputStateEmpty *newState,
                   McBopomofo::InputState *state);
  void handleEmptyIgnoringPrevious(
      McBopomofo::InputStateEmptyIgnoringPrevious *newState,
      McBopomofo::InputState *state);
  void handleCommitting(McBopomofo::InputStateCommitting *newState,
                        McBopomofo::InputState *state);
  void handleInputting(McBopomofo::InputStateInputting *newState,
                       McBopomofo::InputState *state);
  void handleCandidates(McBopomofo::InputStateChoosingCandidate *newState,
                        McBopomofo::InputState *state);
};

class McBopomofoEngineFactory : public fcitx::AddonFactory {
  fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
    FCITX_UNUSED(manager);
    return new McBopomofoEngine();
  }
};

#endif // _FCITX5_MCBOPOMOFO_MCBOPOMOFO_H_
