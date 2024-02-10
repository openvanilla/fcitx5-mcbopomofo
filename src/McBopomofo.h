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

#ifndef SRC_MCBOPOMOFO_H_
#define SRC_MCBOPOMOFO_H_

#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/action.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <fcitx/menu.h>
#include <fcitx/statusarea.h>

#include <memory>
#include <string>
#include <type_traits>

#include "InputMode.h"
#include "InputState.h"
#include "KeyHandler.h"
#include "LanguageModelLoader.h"

namespace McBopomofo {

enum class BopomofoKeyboardLayout {
  Standard,
  Eten,
  Hsu,
  Et26,
  HanyuPinyin,
  IBM
};
FCITX_CONFIG_ENUM_NAME_WITH_I18N(BopomofoKeyboardLayout, N_("standard"),
                                 N_("eten"), N_("hsu"), N_("et26"),
                                 N_("hanyupinyin"), N_("ibm"));

enum class SelectionKeys {
  Key_123456789,
  Key_asdfghjkl,
  Key_asdfzxcvb,
};
FCITX_CONFIG_ENUM_NAME_WITH_I18N(SelectionKeys, N_("123456789"),
                                 N_("asdfghjkl"), N_("asdfzxcvb"));

enum class CandidateLayoutHint { NotSet, Vertical, Horizontal };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(CandidateLayoutHint, N_("Not Set"),
                                 N_("Vertical"), N_("Horizontal"));

enum class SelectPhrase { BeforeCursor, AfterCursor };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(SelectPhrase, N_("before_cursor"),
                                 N_("after_cursor"));

enum class ShiftLetterKeys { DirectlyOutputUppercase, PutLowercaseToBuffer };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(ShiftLetterKeys,
                                 N_("directly_output_uppercase"),
                                 N_("put_lowercase_to_buffer"));

constexpr char kDefaultOpenFileWith[] = "xdg-open";
constexpr char kDefaultAddPhraseHookPath[] =
    "/usr/share/fcitx5/data/mcbopomofo-add-phrase-hook.sh";

FCITX_CONFIG_ENUM_NAME_WITH_I18N(KeyHandlerCtrlEnter, N_("disabled"),
                                 N_("output_bpmf_reading"),
                                 N_("output_html_ruby_text"));

FCITX_CONFIGURATION(
    McBopomofoConfig,
    // Keyboard layout: standard, eten, etc.
    fcitx::OptionWithAnnotation<BopomofoKeyboardLayout,
                                BopomofoKeyboardLayoutI18NAnnotation>
        bopomofoKeyboardLayout{this, "BopomofoKeyboardLayout",
                               _("Bopomofo Keyboard Layout"),
                               BopomofoKeyboardLayout::Standard};

    // Candidate layout.
    fcitx::OptionWithAnnotation<CandidateLayoutHint,
                                CandidateLayoutHintI18NAnnotation>
        candidateLayout{this, "CandidateLayout", _("Candidate List Layout"),
                        CandidateLayoutHint::NotSet};

    // Select selection keys.
    fcitx::OptionWithAnnotation<SelectionKeys, SelectionKeysI18NAnnotation>
        selectionKeys{this, "SelectionKeys", _("Selection Keys"),
                      SelectionKeys::Key_123456789};

    // Select the phrase as candidate before or after the cursor.
    fcitx::OptionWithAnnotation<SelectPhrase, SelectPhraseI18NAnnotation>
        selectPhrase{this, "SelectPhrase", _("Show Candidate Phrase"),
                     SelectPhrase::BeforeCursor};

    // Move the cursor at the end of the selected candidate phrase.
    fcitx::Option<bool> moveCursorAfterSelection{
        this, "MoveCursorAfterSelection", _("Move cursor after selection"),
        false};

    // ESC key clears entire composing buffer.
    fcitx::Option<bool> escKeyClearsEntireComposingBuffer{
        this, "EscKeyClearsEntireComposingBuffer",
        _("ESC key clears entire composing buffer"), false};

    // Shift + letter keys.
    fcitx::OptionWithAnnotation<ShiftLetterKeys, ShiftLetterKeysI18NAnnotation>
        shiftLetterKeys{this, "ShiftLetterKeys", _("Shift + Letter Keys"),
                        ShiftLetterKeys::DirectlyOutputUppercase};

    // Ctrl + enter keys.
    fcitx::OptionWithAnnotation<KeyHandlerCtrlEnter,
                                KeyHandlerCtrlEnterI18NAnnotation>
        ctrlEnterKeys{this, "KeyHandlerCtrlEnter", _("Control + Enter Key"),
                      KeyHandlerCtrlEnter::Disabled};

    // The app to open custom phrase text files.
    fcitx::Option<std::string> openUserPhraseFilesWith{
        this, "OpenUserPhraseFilesWith", _("Open User Phrase Files With"),
        kDefaultOpenFileWith};

    // The path of the hook-script to add a phrase.
    fcitx::Option<std::string> addScriptHookPath{this, "AddScriptHookPath",
                                                 _("Add Phrase Hook Path"),
                                                 kDefaultAddPhraseHookPath};

    fcitx::Option<bool> addScriptHookEnabled{
        this, "AddScriptHookEnabled",
        _("Run the hook script after adding a phrase"), false};

    fcitx::Option<bool> associatedPhrasesEnabled{
        this, "AssociatedPhrasesEnabled", _("Enable Associated Phrases"),
        false};);

class McBopomofoEngine : public fcitx::InputMethodEngine {
 public:
  explicit McBopomofoEngine(fcitx::Instance* instance);
  fcitx::Instance* instance() { return instance_; }

  void activate(const fcitx::InputMethodEntry& entry,
                fcitx::InputContextEvent& event) override;
  void reset(const fcitx::InputMethodEntry& entry,
             fcitx::InputContextEvent& event) override;
  void keyEvent(const fcitx::InputMethodEntry& entry,
                fcitx::KeyEvent& keyEvent) override;

  const fcitx::Configuration* getConfig() const override;
  void setConfig(const fcitx::RawConfig& config) override;
  void reloadConfig() override;

 private:
  FCITX_ADDON_DEPENDENCY_LOADER(chttrans, instance_->addonManager());
  fcitx::Instance* instance_;

  bool handleCandidateKeyEvent(
      fcitx::InputContext* context, fcitx::Key key, fcitx::Key origKey,
      fcitx::CommonCandidateList* candidateList,
      const McBopomofo::KeyHandler::StateCallback& stateCallback,
      const McBopomofo::KeyHandler::ErrorCallback& errorCallback);

  // Handles state transitions.
  void enterNewState(fcitx::InputContext* context,
                     std::unique_ptr<InputState> newState);

  // Methods below enterNewState raw pointers as they don't affect ownership.
  void handleEmptyState(fcitx::InputContext* context, InputState* prev,
                        InputStates::Empty* current);
  void handleEmptyIgnoringPreviousState(
      fcitx::InputContext* context, InputState* prev,
      InputStates::EmptyIgnoringPrevious* current);
  void handleCommittingState(fcitx::InputContext* context, InputState* prev,
                             InputStates::Committing* current);
  void handleInputtingState(fcitx::InputContext* context, InputState* prev,
                            InputStates::Inputting* current);
  void handleCandidatesState(fcitx::InputContext* context, InputState* prev,
                             InputState* current);
  void handleMarkingState(fcitx::InputContext* context, InputState* prev,
                          InputStates::Marking* current);
  void handleChineseNumberState(fcitx::InputContext* context, InputState*,
                                InputStates::ChineseNumber* current);

  // Helpers.

  // Updates the preedit with a not-empty state's composing buffer and cursor
  // index.
  void updatePreedit(fcitx::InputContext* context,
                     InputStates::NotEmpty* state);

  fcitx::CandidateLayoutHint getCandidateLayoutHint() const;

  std::shared_ptr<LanguageModelLoader> languageModelLoader_;
  std::shared_ptr<KeyHandler> keyHandler_;
  std::unique_ptr<InputState> state_;
  McBopomofoConfig config_;
  fcitx::KeyList selectionKeys_;

  std::unique_ptr<fcitx::SimpleAction> editUserPhrasesAction_;
  std::unique_ptr<fcitx::SimpleAction> excludedPhrasesAction_;
};

class McBopomofoEngineFactory : public fcitx::AddonFactory {
  fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
    FCITX_UNUSED(manager);
    return new McBopomofoEngine(manager->instance());
  }
};

}  // namespace McBopomofo

#endif  // SRC_MCBOPOMOFO_H_
