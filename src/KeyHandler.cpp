#include "KeyHandler.h"

namespace McBopomofo {
KeyHandler::KeyHandler() {
  _bpmfReadingBuffer = new Formosa::Mandarin::BopomofoReadingBuffer(
      Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout());
  _languageModel = new McBopomofo::McBopomofoLM();
  // _userOverrideModel = new McBopomofo::UserOverrideModel;
  _builder = new Formosa::Gramambular::BlockReadingBuilder(_languageModel);
  _builder->setJoinSeparator("-");
}

KeyHandler::~KeyHandler() {
  delete _bpmfReadingBuffer;
  delete _languageModel;
  delete _userOverrideModel;
  delete _builder;
}

bool KeyHandler::handle(
    fcitx::KeyEvent &keyEvent, McBopomofo::InputState *state,
    std::function<void(McBopomofo::InputState)> stateCallback,
    std::function<void(void)> errorCallback) {

  if (dynamic_cast<McBopomofo::InputStateEmpty *>(state)) {
    return false;
  }
  return false;
}

} // namespace McBopomofo
