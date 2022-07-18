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

#ifndef SRC_ABSTRACT_KEYHANDLER_H_
#define SRC_ABSTRACT_KEYHANDLER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Engine/gramambular2/language_model.h"
#include "Engine/gramambular2/reading_grid.h"
#include "InputState.h"
#include "Key.h"
#include "LanguageModelLoader.h"
#include "Mandarin.h"
#include "McBopomofoLM.h"
#include "UserOverrideModel.h"

namespace McBopomofo {

enum class KeyHandlerCtrlEnter {
  Disabled,
  OutputBpmfReadings,
  OutputHTMLRubyText
};

class AbstractKeyHandler {
 public:
  virtual ~AbstractKeyHandler() = default;

  using StateCallback =
      std::function<void(std::unique_ptr<McBopomofo::InputState>)>;
  using ErrorCallback = std::function<void(void)>;

  // Given a fcitx5 KeyEvent and the current state, invokes the stateCallback if
  // a new state is entered, or errorCallback will be invoked. Returns true if
  // the key should be absorbed, signaling that the key is accepted and handled,
  // or false if the event should be let pass through.
  virtual bool handle(Key key, McBopomofo::InputState* state,
                      const StateCallback& stateCallback,
                      const ErrorCallback& errorCallback) = 0;

  // Candidate selected. Can assume the context is in a candidate state.
  virtual void candidateSelected(
      const InputStates::ChoosingCandidate::Candidate& candidate,
      const StateCallback& stateCallback) = 0;

  // Candidate panel canceled. Can assume the context is in a candidate state.
  virtual void candidatePanelCancelled(const StateCallback& stateCallback) = 0;

  virtual void reset() = 0;

#pragma region Settings

  // Sets the Bopomofo keyboard layout.
  virtual void setKeyboardLayout(
      const Formosa::Mandarin::BopomofoKeyboardLayout* layout) = 0;

  // Sets if we should select phrase after cursor as candidate.
  virtual void setSelectPhraseAfterCursorAsCandidate(bool flag) = 0;

  // Sets if we should move cursor after selection.
  virtual void setMoveCursorAfterSelection(bool flag) = 0;

  // Sets if we should put lowercase letters into the composing buffer.
  virtual void setPutLowercaseLettersToComposingBuffer(bool flag) = 0;

  // Sets if the ESC key clears entire composing buffer.
  virtual void setEscKeyClearsEntireComposingBuffer(bool flag) = 0;

  virtual void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior) = 0;

  virtual void setOnAddNewPhrase(
      std::function<void(const std::string&)> onAddNewPhrase) = 0;
};

}  // namespace McBopomofo

#endif  // SRC_ABSTRACT_KEYHANDLER_H_
