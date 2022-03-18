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

#ifndef SRC_KEYHANDLER_H_
#define SRC_KEYHANDLER_H_

#include <fcitx/inputmethodengine.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Gramambular.h"
#include "InputState.h"
#include "Mandarin.h"
#include "McBopomofoLM.h"
#include "UserOverrideModel.h"

namespace McBopomofo {
class KeyHandler {
 public:
  explicit KeyHandler(
      std::shared_ptr<Formosa::Gramambular::LanguageModel> languageModel);

  // Given a fcitx5 KeyEvent and the current state, invokes the stateCallback if
  // a new state is entered, or errorCallback will be invoked. Returns true if
  // the key should be absorbed, signaling that the key is accepted and handled,
  // or false if the event should be let pass through.
  bool handle(const fcitx::KeyEvent& keyEvent, McBopomofo::InputState* state,
              std::function<void(std::unique_ptr<McBopomofo::InputState>)>
                  stateCallback,
              std::function<void(void)> errorCallback);

 private:
  Formosa::Mandarin::BopomofoReadingBuffer bopomofoReadingBuffer_;

  // language model
  std::shared_ptr<Formosa::Gramambular::LanguageModel> languageModel_;

  std::unique_ptr<Formosa::Gramambular::BlockReadingBuilder> builder_;

  // latest walked path (trellis) using the Viterbi algorithm
  std::vector<Formosa::Gramambular::NodeAnchor> walkedNodes_;
};

}  // namespace McBopomofo

#endif  // SRC_KEYHANDLER_H_
