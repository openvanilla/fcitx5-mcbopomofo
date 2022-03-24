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

  using StateCallback =
      std::function<void(std::unique_ptr<McBopomofo::InputState>)>;
  using ErrorCallback = std::function<void(void)>;

  // Given a fcitx5 KeyEvent and the current state, invokes the stateCallback if
  // a new state is entered, or errorCallback will be invoked. Returns true if
  // the key should be absorbed, signaling that the key is accepted and handled,
  // or false if the event should be let pass through.
  bool handle(fcitx::Key key, McBopomofo::InputState* state,
              StateCallback stateCallback, ErrorCallback errorCallback);

  // Candidate selected. Can assume the context is in a candidate state.
  void candidateSelected(const std::string& candidate,
                         StateCallback stateCallback);
  // Candidate panel canceled. Can assume the context is in a candidate state.
  void candidatePanelCancelled(StateCallback stateCallback);

  void reset();

  // Sets the Bopomofo keyboard layout.
  void setKeyboardLayout(
      const Formosa::Mandarin::BopomofoKeyboardLayout* layout);

  // Sets select phrase after cursor as candidate.
  void setSelectPhraseAfterCursorAsCandidate(bool flag);

 private:
  bool handleCursorKeys(fcitx::Key key, McBopomofo::InputState* state,
                        StateCallback stateCallback,
                        ErrorCallback errorCallback);
  bool handleDeleteKeys(fcitx::Key key, McBopomofo::InputState* state,
                        StateCallback stateCallback,
                        ErrorCallback errorCallback);
  bool handlePunctuation(std::string punctuationUnigramKey,
                         StateCallback stateCallback,
                         ErrorCallback errorCallback);

  std::unique_ptr<InputStates::Inputting> buildInputtingState();
  std::unique_ptr<InputStates::ChoosingCandidate> bulidChoosingCandidateState(
      InputStates::NotEmpty* nonEmptyState);

  // Returns the text that needs to be evicted from the walked grid due to the
  // grid now being overflown with the recently added reading, then walk the
  // grid.
  std::string popEvictedTextAndWalk();

  // Compute the actual candidate cursor index.
  size_t actualCandidateCursorIndex();

  // Pin a node with a fixed unigram value, usually a candidate.
  void pinNode(const std::string& candidate);

  void walk();

  Formosa::Mandarin::BopomofoReadingBuffer reading_;

  // language model
  std::shared_ptr<Formosa::Gramambular::LanguageModel> languageModel_;

  std::unique_ptr<Formosa::Gramambular::BlockReadingBuilder> builder_;

  // latest walked path (trellis) using the Viterbi algorithm
  std::vector<Formosa::Gramambular::NodeAnchor> walkedNodes_;

  bool selectPhraseAfterCursorAsCandidate_;
};

}  // namespace McBopomofo

#endif  // SRC_KEYHANDLER_H_
