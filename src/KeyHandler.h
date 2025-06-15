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

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "DictionaryService.h"
#include "Engine/Mandarin/Mandarin.h"
#include "Engine/McBopomofoLM.h"
#include "Engine/UserOverrideModel.h"
#include "Engine/gramambular2/language_model.h"
#include "Engine/gramambular2/reading_grid.h"
#include "InputMode.h"
#include "InputState.h"
#include "Key.h"
#include "LanguageModelLoader.h"

namespace McBopomofo {

enum class KeyHandlerCtrlEnter {
  Disabled,
  OutputBpmfReadings,
  OutputHTMLRubyText,
  OutputHanyuPinyin,
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
  using SelectCurrentCandidateCallback = std::function<void(void)>;

  // Given a fcitx5 KeyEvent and the current state, invokes the stateCallback if
  // a new state is entered, or errorCallback will be invoked. Returns true if
  // the key should be absorbed, signaling that the key is accepted and handled,
  // or false if the event should be let pass through.
  bool handle(Key key, McBopomofo::InputState* state,
              StateCallback stateCallback, ErrorCallback errorCallback);

  bool handleAssociatedPhrases(InputStates::Inputting* state,
                               StateCallback stateCallback,
                               ErrorCallback errorCallback, bool useShiftKey);

  // Candidate selected. Can assume the context is in a candidate state.
  void candidateSelected(
      const InputStates::ChoosingCandidate::Candidate& candidate,
      size_t originalCursor, StateCallback stateCallback);

  void candidateAssociatedPhraseSelected(
      size_t index, const InputStates::ChoosingCandidate::Candidate& candidate,
      const std::string& selectedReading, const std::string& selectedValue,
      const StateCallback& stateCallback);

  void dictionaryServiceSelected(std::string phrase, size_t index,
                                 InputState* currentState,
                                 StateCallback stateCallback);

  // Candidate panel canceled. Can assume the context is in a candidate state.
  void candidatePanelCancelled(size_t originalCursor,
                               StateCallback stateCallback);

  // Workaround for the Traditional Bopomofo mode.
  bool handleCandidateKeyForTraditionalBopomofoIfRequired(
      Key key, SelectCurrentCandidateCallback SelectCurrentCandidateCallback,
      StateCallback stateCallback, ErrorCallback errorCallback);

  void boostPhrase(const std::string& reading, const std::string& value);

  void excludePhrase(const std::string& reading, const std::string& value);

  void reset();

#pragma region Dictionary Services

  bool hasDictionaryServices();

  std::unique_ptr<InputStates::SelectingDictionary>
  buildSelectingDictionaryState(
      std::unique_ptr<InputStates::NotEmpty> nonEmptyState,
      const std::string& selectedPhrase, size_t selectedIndex);

#pragma endregion Dictionary Services

#pragma region Settings

  McBopomofo::InputMode inputMode();

  // Sets the input mode.
  void setInputMode(McBopomofo::InputMode mode);

  // Sets the Bopomofo keyboard layout.
  void setKeyboardLayout(
      const Formosa::Mandarin::BopomofoKeyboardLayout* layout);

  // Sets if we should select phrase after cursor as candidate.
  void setSelectPhraseAfterCursorAsCandidate(bool flag);

  // Sets if we should move cursor after selection.
  void setMoveCursorAfterSelection(bool flag);

  // Sets if we should put lowercased letters into the composing buffer.
  void setPutLowercaseLettersToComposingBuffer(bool flag);

  // Sets if the ESC key clears entire composing buffer.
  void setEscKeyClearsEntireComposingBuffer(bool flag);

  // Sets if the Shift + Enter key is enabled.
  void setShiftEnterEnabled(bool flag);

  // Sets the behaviour of the Ctrl + Enter key
  void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior);

  // Sets if associated phrases is enabled or not.
  void setAssociatedPhrasesEnabled(bool enabled);

  // Sets if half width punctuation is enabled or not.
  void setHalfWidthPunctuationEnabled(bool enabled);

  // Sets the lambda for adding the phrases.
  void setOnAddNewPhrase(
      std::function<void(const std::string&)> onAddNewPhrase);

  //  Sets whether repeated punctuation characters should be used to select
  //  candidates.
  void setRepeatedPunctuationToSelectCandidateEnabled(bool enabled);

  // Sets whether the space key should be used to choose candidates.
  void setChooseCandidateUsingSpace(bool enabled);

  // Compute the actual candidate cursor index based on the current index.
  size_t actualCandidateCursorIndex();
  // Compute the actual candidate cursor index.
  size_t computeActualCandidateCursorIndex(size_t index);

  // Get the current cursor index.
  size_t candidateCursorIndex();
  // Set the current cursor index.
  void setCandidateCursorIndex(size_t newCursor);

  // Build an Inputting state.
  std::unique_ptr<InputStates::Inputting> buildInputtingState();

  // Build a Choosing Candidate state.
  std::unique_ptr<InputStates::ChoosingCandidate> buildChoosingCandidateState(
      InputStates::NotEmpty* nonEmptyState, size_t originalCursor);

  // Reading joiner for retrieving unigrams from the language model.
  static constexpr char kJoinSeparator[] = "-";

  // Build an Associated Phrase state. The prefixCursorIndex is where the
  // prefix node is actually located in the grid.
  std::unique_ptr<InputStates::AssociatedPhrases> buildAssociatedPhrasesState(
      std::unique_ptr<InputStates::NotEmpty> previousState,
      size_t prefixCursorIndex, std::string prefixCombinedReading,
      std::string prefixValue, size_t selectedCandidateIndex, bool useShiftKey);

  // Build an Associated Phrase state. The candidateCursorIndex is where the
  // user-visible cursor was *before* the ChoosingCandidateState was entered,
  // which actually means the place where the candidate were collected. You
  // must call this method if you want to enter the Associated Phrase, since
  // the candidate cursor position is not always the prefix node position
  // (consider the Hanyin cursor mode)!
  std::unique_ptr<InputStates::AssociatedPhrases>
  buildAssociatedPhrasesStateFromCandidateChoosingState(
      std::unique_ptr<InputStates::NotEmpty> previousState,
      size_t candidateCursorIndex, std::string prefixCombinedReading,
      std::string prefixValue, size_t selectedCandidateIndex);

  // Build an Associated Phrases Plain state by the given key. It could be
  // nullptr when there is no associated phrases.
  std::unique_ptr<InputStates::AssociatedPhrasesPlain>
  buildAssociatedPhrasesPlainState(const std::string& reading,
                                   const std::string& value);

#pragma endregion Settings

 private:
  bool handleChineseNumber(Key key,
                           McBopomofo::InputStates::ChineseNumber* state,
                           StateCallback stateCallback,
                           ErrorCallback errorCallback);
  bool handleEnclosingNumber(Key key,
                             McBopomofo::InputStates::EnclosingNumber* state,
                             StateCallback stateCallback,
                             ErrorCallback errorCallback);

  bool handleTabKey(bool isShiftPressed, McBopomofo::InputState* state,
                    const StateCallback& stateCallback,
                    const ErrorCallback& errorCallback);
  bool handleCursorKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handleDeleteKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handlePunctuation(const std::string& punctuationUnigramKey,
                         McBopomofo::InputState* state,
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
  std::string getHanyuPinyin();

  // Build a Marking state, ranging from beginCursorIndex to the current builder
  // cursor. It doesn't matter if the beginCursorIndex is behind or after the
  // builder cursor.
  std::unique_ptr<InputStates::Marking> buildMarkingState(
      size_t beginCursorIndex);

  // Pin a node with a fixed unigram value, usually a candidate.
  void pinNode(size_t originalCursor,
               const InputStates::ChoosingCandidate::Candidate& candidate,
               bool useMoveCursorAfterSelectionSetting = true);

  // Pin a node with an associated phrase; an associated phrase has a prefix
  // that is either the current node at the cursor, or a new "override" phrase
  // from the choosing-candidate state; the actual associated phrase will also
  // contain the prefix. This allows the following two scenarios:
  //
  // (1) the current walk is 得 and we want to pin the phrase 得到; in this
  //     case, the prefixReading is ㄉㄜˊ and prefixValue is 得, and the
  //     associated phrase's reading and value are ㄉㄜˊ-ㄉㄠˋ and 得到
  //     respectively.
  // (2) the current walk is 得 but we want to pin the phrase 德性, coming from
  //     the choosing-candidate state; in this case, the prefix reading and
  //     value is now ㄉㄜˊ and 德, and the associated phrase is ㄉㄜˊ-ㄒㄧㄥˋ
  //     and 德性 respectively.
  void pinNodeWithAssociatedPhrase(size_t prefixCursorIndex,
                                   const std::string& prefixReading,
                                   const std::string& prefixValue,
                                   const std::string& associatedPhraseReading,
                                   const std::string& associatedPhraseValue);

  void walk();

  std::shared_ptr<Formosa::Gramambular2::LanguageModel> lm_;
  Formosa::Gramambular2::ReadingGrid grid_;
  std::shared_ptr<UserPhraseAdder> userPhraseAdder_;
  std::unique_ptr<LocalizedStrings> localizedStrings_;

  UserOverrideModel userOverrideModel_;
  Formosa::Mandarin::BopomofoReadingBuffer reading_;
  Formosa::Gramambular2::ReadingGrid::WalkResult latestWalk_;
  std::shared_ptr<DictionaryServices> dictionaryServices_;

#pragma region Settings

  McBopomofo::InputMode inputMode_ = McBopomofo::InputMode::McBopomofo;
  bool selectPhraseAfterCursorAsCandidate_ = false;
  bool moveCursorAfterSelection_ = false;
  bool putLowercaseLettersToComposingBuffer_ = false;
  bool escKeyClearsEntireComposingBuffer_ = false;
  bool shiftEnterEnabled_ = true;
  bool associatedPhrasesEnabled_ = false;
  bool halfWidthPunctuationEnabled_ = false;
  bool repeatedPunctuationToSelectCandidateEnabled_ = false;
  bool chooseCandidateUsingSpace_ = true;
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
