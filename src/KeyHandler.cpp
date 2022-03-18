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

#include "KeyHandler.h"

#include <utility>

// TODO(unassigned): Remove this after everything is implemented.
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace McBopomofo {

constexpr const char* kJoinSeparator = "-";

KeyHandler::KeyHandler(
    std::unique_ptr<Formosa::Gramambular::LanguageModel> languageModel)
    : bopomofoReadingBuffer_(
          Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout()) {
  languageModel_ = std::move(languageModel);
  builder_ = std::make_unique<Formosa::Gramambular::BlockReadingBuilder>(
      languageModel_.get());
  builder_->setJoinSeparator(kJoinSeparator);
}

bool KeyHandler::handle(
    const fcitx::KeyEvent& keyEvent, McBopomofo::InputState* state,
    std::function<void(std::unique_ptr<McBopomofo::InputState>)> stateCallback,
    std::function<void(void)> errorCallback) {
  if (dynamic_cast<McBopomofo::InputStateEmpty*>(state)) {
    return false;
  }
  return false;
}

#pragma GCC diagnostic pop

}  // namespace McBopomofo
