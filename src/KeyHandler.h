#ifndef _FCITX5_MCBOPOMOFO_KEYHANDLER_H_
#define _FCITX5_MCBOPOMOFO_KEYHANDLER_H_

#include "Gramambular.h"
#include "InputState.h"
#include "Mandarin.h"
#include "McBopomofoLM.h"
#include "UserOverrideModel.h"
#include <fcitx/inputmethodengine.h>
#include <functional>
#include <string>
#include <vector>

namespace McBopomofo {
class KeyHandler {

public:
  KeyHandler();
  ~KeyHandler();
  bool handle(fcitx::KeyEvent &keyEvent, McBopomofo::InputState *state,
              std::function<void(McBopomofo::InputState)> stateCallback,
              std::function<void(void)> errorCallback);

private:
  Formosa::Mandarin::BopomofoReadingBuffer *_bpmfReadingBuffer;

  // language model
  McBopomofo::McBopomofoLM *_languageModel;

  // user override model
  McBopomofo::UserOverrideModel *_userOverrideModel;

  // the grid (lattice) builder for the unigrams (and bigrams)
  Formosa::Gramambular::BlockReadingBuilder *_builder;

  // latest walked path (trellis) using the Viterbi algorithm
  std::vector<Formosa::Gramambular::NodeAnchor> _walkedNodes;
};

} // namespace McBopomofo

#endif
