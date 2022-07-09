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

class KeyHandler {
 public:
  class LocalizedStrings;

  explicit KeyHandler(
      std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel,
      std::shared_ptr<UserPhraseAdder> userPhraseAdder,
      std::unique_ptr<LocalizedStrings> localizedStrings);

  using StateCallback =
      std::function<void(std::unique_ptr<McBopomofo::InputState>)>;
  using ErrorCallback = std::function<void(void)>;

  // Given a fcitx5 KeyEvent and the current state, invokes the stateCallback if
  // a new state is entered, or errorCallback will be invoked. Returns true if
  // the key should be absorbed, signaling that the key is accepted and handled,
  // or false if the event should be let pass through.
  bool handle(Key key, McBopomofo::InputState* state,
              const StateCallback& stateCallback,
              const ErrorCallback& errorCallback);

  // Candidate selected. Can assume the context is in a candidate state.
  void candidateSelected(
      const InputStates::ChoosingCandidate::Candidate& candidate,
      const StateCallback& stateCallback);
  // Candidate panel canceled. Can assume the context is in a candidate state.
  void candidatePanelCancelled(const StateCallback& stateCallback);

  void reset();

#pragma region Settings

  // Sets the Bopomofo keyboard layout.
  void setKeyboardLayout(
      const Formosa::Mandarin::BopomofoKeyboardLayout* layout);

  // Sets if we should select phrase after cursor as candidate.
  void setSelectPhraseAfterCursorAsCandidate(bool flag);

  // Sets if we should move cursor after selection.
  void setMoveCursorAfterSelection(bool flag);

  // Sets if we should put lowercasesd letters into the composing buffer.
  void setPutLowercaseLettersToComposingBuffer(bool flag);

  /// Sets if the ESC key clears entire composing buffer.
  void setEscKeyClearsEntireComposingBuffer(bool flag);

  void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior);

  void setOnAddNewPhrase(
      std::function<void(const std::string&)> onAddNewPhrase);

  // Reading joiner for retrieving unigrams from the language model.
  static constexpr char kJoinSeparator[] = "-";

#pragma endregion Settings

 private:
  bool handleTabKey(Key key, McBopomofo::InputState* state,
                    const StateCallback& stateCallback,
                    const ErrorCallback& errorCallback);
  bool handleCursorKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handleDeleteKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handlePunctuation(const std::string& punctuationUnigramKey,
                         const StateCallback& stateCallback,
                         const ErrorCallback& errorCallback);

  // Get the head and the tail of current composed string, separated by the
  // current cursor.
  struct ComposedString {
    std::string head;
    std::string tail;
    // Any tooltip during the build process.
    std::string tooltip;
  };
  ComposedString getComposedString(size_t builderCursor);
  std::string getHTMLRubyText();

  std::unique_ptr<InputStates::Inputting> buildInputtingState();
  std::unique_ptr<InputStates::ChoosingCandidate> buildChoosingCandidateState(
      InputStates::NotEmpty* nonEmptyState);

  // Build a Marking state, ranging from beginCursorIndex to the current builder
  // cursor. It doesn't matter if the beginCursorIndex is behind or after the
  // builder cursor.
  std::unique_ptr<InputStates::Marking> buildMarkingState(
      size_t beginCursorIndex);

  // Compute the actual candidate cursor index.
  size_t actualCandidateCursorIndex();

  // Pin a node with a fixed unigram value, usually a candidate.
  void pinNode(const InputStates::ChoosingCandidate::Candidate& candidate,
               bool useMoveCursorAfterSelectionSetting = true);

  void walk();

  std::shared_ptr<Formosa::Gramambular2::LanguageModel> lm_;
  Formosa::Gramambular2::ReadingGrid grid_;
  std::shared_ptr<UserPhraseAdder> userPhraseAdder_;
  std::unique_ptr<LocalizedStrings> localizedStrings_;

  UserOverrideModel userOverrideModel_;
  Formosa::Mandarin::BopomofoReadingBuffer reading_;
  Formosa::Gramambular2::ReadingGrid::WalkResult latestWalk_;

#pragma region Settings

  bool selectPhraseAfterCursorAsCandidate_ = false;
  bool moveCursorAfterSelection_ = false;
  bool putLowercaseLettersToComposingBuffer_ = false;
  bool escKeyClearsEntireComposingBuffer_ = false;
  KeyHandlerCtrlEnter ctrlEnterKey_ = KeyHandlerCtrlEnter::Disabled;
  std::function<void(const std::string&)> onAddNewPhrase_;

#pragma endregion Settings

 public:
  // Localization helper. We use dependency injection, that is, passing an
  // instance of the class when constructing KeyHandler, so that KeyHandler
  // itself is not concerned with how localization is implemented.
  class LocalizedStrings {
   public:
    virtual ~LocalizedStrings() = default;

    // Reference string: "Cursor is between syllables {0} and {1}"
    virtual std::string cursorIsBetweenSyllables(
        const std::string& prevReading, const std::string& nextReading) = 0;
    // Reference string: "{0} syllables required"
    virtual std::string syllablesRequired(size_t syllables) = 0;
    // Reference string: "{0} syllables maximum"
    virtual std::string syllablesMaximum(size_t syllables) = 0;
    // Reference string: "phrase already exists"
    virtual std::string phraseAlreadyExists() = 0;
    // Reference string: "press Enter to add the phrase"
    virtual std::string pressEnterToAddThePhrase() = 0;
    // Reference string: "Marked: {0}, syllables: {1}, {2}"
    virtual std::string markedWithSyllablesAndStatus(
        const std::string& marked, const std::string& readingUiText,
        const std::string& status) = 0;
  };
};

}  // namespace McBopomofo

#endif  // SRC_KEYHANDLER_H_
