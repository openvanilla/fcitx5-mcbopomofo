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

#include <string>

#include "KeyHandler.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace McBopomofo {

constexpr char kTestDataPath[] = "mcbopomofo-test-data.txt";

class MockUserPhraseAdder : public UserPhraseAdder {
 public:
  void addUserPhrase(const std::string_view&,
                     const std::string_view&) override {}
  void removeUserPhrase(const std::string_view&,
                        const std::string_view&) override {}
};

class MockLocalizedString : public KeyHandler::LocalizedStrings {
 public:
  std::string cursorIsBetweenSyllables(
      const std::string& prevReading, const std::string& nextReading) override {
    return std::string("between ") + prevReading + " and " + nextReading;
  }

  std::string syllablesRequired(size_t syllables) override {
    return std::to_string(syllables) + " syllables required";
  }

  std::string syllablesMaximum(size_t syllables) override {
    return std::to_string(syllables) + " syllables maximum";
  }

  std::string phraseAlreadyExists() override { return "phrase already exists"; }

  std::string pressEnterToAddThePhrase() override {
    return "press Enter to add the phrase";
  }

  std::string markedWithSyllablesAndStatus(const std::string& marked,
                                           const std::string& readingUiText,
                                           const std::string& status) override {
    return std::string("Marked: ") + marked + ", syllables: " + readingUiText +
           ", " + status;
  }
};

class KeyHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    languageModel_ = std::make_shared<ParselessLM>();
    bool result = languageModel_->open(kTestDataPath);
    ASSERT_TRUE(result);
    userPhraseAdder_ = std::make_shared<MockUserPhraseAdder>();

    keyHandler_ =
        std::make_unique<KeyHandler>(languageModel_, userPhraseAdder_,
                                     std::make_unique<MockLocalizedString>());
  }

  std::vector<Key> asciiKeys(const std::string& keyString) {
    std::vector<Key> keys;
    for (const char chr : keyString) {
      keys.emplace_back(Key::asciiKey(chr));
    }
    return keys;
  }

  // Given a sequence of keys, return the last state.
  std::unique_ptr<InputState> handleKeySequence(
      const std::vector<Key>& keys, bool expectHandled = true,
      bool expectErrorCallbackAtEnd = false) {
    std::unique_ptr<InputState> state = std::make_unique<InputStates::Empty>();

    bool handled = false;
    bool errorCallbackInvoked = false;

    for (const Key& key : keys) {
      errorCallbackInvoked = false;
      handled = keyHandler_->handle(
          key, state.get(),
          [&state](std::unique_ptr<McBopomofo::InputState> newState) {
            if (dynamic_cast<InputStates::EmptyIgnoringPrevious*>(
                    newState.get()) != nullptr) {
              // Transition required by the contract of EmptyIgnoringPrevious.
              state = std::make_unique<InputStates::Empty>();
            } else {
              state = std::move(newState);
            }
          },
          [&errorCallbackInvoked]() { errorCallbackInvoked = true; });
    }

    EXPECT_EQ(expectHandled, handled);
    EXPECT_EQ(expectErrorCallbackAtEnd, errorCallbackInvoked);
    return state;
  }

  std::shared_ptr<ParselessLM> languageModel_;
  std::shared_ptr<UserPhraseAdder> userPhraseAdder_;
  std::unique_ptr<KeyHandler> keyHandler_;
};

TEST_F(KeyHandlerTest, EmptyKeyNotHandled) {
  bool stateCallbackInvoked = false;
  bool errorCallbackInvoked = false;
  auto emptyState = std::make_unique<InputStates::Empty>();
  bool handled = keyHandler_->handle(
      Key(), emptyState.get(),
      [&stateCallbackInvoked](std::unique_ptr<McBopomofo::InputState>) {
        stateCallbackInvoked = true;
      },
      [&errorCallbackInvoked]() { errorCallbackInvoked = true; });
  EXPECT_FALSE(stateCallbackInvoked);
  EXPECT_FALSE(errorCallbackInvoked);
  EXPECT_FALSE(handled);
}

TEST_F(KeyHandlerTest, EmptyPassthrough) {
  auto endState = handleKeySequence(asciiKeys(" "), /*expectHandled=*/false);
  auto emptyState = dynamic_cast<InputStates::Empty*>(endState.get());
  ASSERT_TRUE(emptyState != nullptr);
}

TEST_F(KeyHandlerTest, SimpleReading) {
  auto endState = handleKeySequence(asciiKeys("1"));
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ㄅ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ㄅ"));
}

TEST_F(KeyHandlerTest, SimpleReadingPlusUnhandledKey) {
  auto keys = asciiKeys("1");
  keys.emplace_back(Key::namedKey(Key::KeyName::LEFT));
  auto endState = handleKeySequence(keys, /*expectHandled=*/true,
                                    /*expectErrorCallbackAtEnd=*/true);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ㄅ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ㄅ"));
}

TEST_F(KeyHandlerTest, FullSyllable) {
  auto endState = handleKeySequence(asciiKeys("5j/"));
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ㄓㄨㄥ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ㄓㄨㄥ"));
}

TEST_F(KeyHandlerTest, FullSyllablesThenCompose) {
  auto endState = handleKeySequence(asciiKeys("5j/ jp6"));
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "中文");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("中文"));
}

TEST_F(KeyHandlerTest, EnterCandidateState) {
  auto endState = handleKeySequence(asciiKeys("5j/ jp6 "));
  auto choosingCandidateState =
      dynamic_cast<InputStates::ChoosingCandidate*>(endState.get());
  ASSERT_TRUE(choosingCandidateState != nullptr);
  ASSERT_EQ(choosingCandidateState->composingBuffer, "中文");
  ASSERT_EQ(choosingCandidateState->cursorIndex, strlen("中文"));

  EXPECT_THAT(choosingCandidateState->candidates,
              testing::Contains(InputStates::ChoosingCandidate::Candidate(
                  "ㄓㄨㄥ-ㄨㄣˊ", "中文")));
}

TEST_F(KeyHandlerTest, CursorMovementLeft) {
  auto keys = asciiKeys("5j/ jp6");
  keys.emplace_back(Key::namedKey(Key::KeyName::LEFT));
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "中文");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("中"));
}

TEST_F(KeyHandlerTest, CursorMovementHome) {
  auto keys = asciiKeys("5j/ jp6");
  keys.emplace_back(Key::namedKey(Key::KeyName::HOME));
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "中文");
  ASSERT_EQ(inputtingState->cursorIndex, 0);
}

TEST_F(KeyHandlerTest, SelectCandidatesBeforeCursor) {
  auto keys = asciiKeys("5j/ jp6");
  keys.emplace_back(Key::namedKey(Key::KeyName::LEFT));
  keys.emplace_back(Key::asciiKey(Key::SPACE));
  auto endState = handleKeySequence(keys);
  auto choosingCandidateState =
      dynamic_cast<InputStates::ChoosingCandidate*>(endState.get());
  ASSERT_TRUE(choosingCandidateState != nullptr);
  ASSERT_EQ(choosingCandidateState->composingBuffer, "中文");
  ASSERT_EQ(choosingCandidateState->cursorIndex, strlen("中"));
  EXPECT_THAT(choosingCandidateState->candidates,
              testing::Contains(
                  InputStates::ChoosingCandidate::Candidate("ㄓㄨㄥ", "中")));
}

TEST_F(KeyHandlerTest, SelectCandidatesAfterCursor) {
  keyHandler_->setSelectPhraseAfterCursorAsCandidate(true);

  auto keys = asciiKeys("5j/ jp6");
  keys.emplace_back(Key::namedKey(Key::KeyName::LEFT));
  keys.emplace_back(Key::asciiKey(Key::SPACE));
  auto endState = handleKeySequence(keys);
  auto choosingCandidateState =
      dynamic_cast<InputStates::ChoosingCandidate*>(endState.get());
  ASSERT_TRUE(choosingCandidateState != nullptr);
  ASSERT_EQ(choosingCandidateState->composingBuffer, "中文");
  ASSERT_EQ(choosingCandidateState->cursorIndex, strlen("中"));
  EXPECT_THAT(choosingCandidateState->candidates,
              testing::Contains(
                  InputStates::ChoosingCandidate::Candidate("ㄨㄣˊ", "文")));
}

TEST_F(KeyHandlerTest, UppercaseLetterCommitComposingBufferByDefault) {
  auto endState = handleKeySequence(asciiKeys("jp6A"));
  auto committingState = dynamic_cast<InputStates::Committing*>(endState.get());
  ASSERT_TRUE(committingState != nullptr);
  // "文" was already committed, so only A is committed.
  ASSERT_EQ(committingState->text, "A");
}

TEST_F(KeyHandlerTest, UppercaseLetterNotHandledIfComposingBufferIsEmpty) {
  auto endState = handleKeySequence(asciiKeys("A"), /*expectHandled=*/false);
  auto emptyState = dynamic_cast<InputStates::Empty*>(endState.get());
  ASSERT_TRUE(emptyState != nullptr);
}

TEST_F(KeyHandlerTest, UppercaseLetterConvertedToLowercaseInComposingBuffer) {
  keyHandler_->setPutLowercaseLettersToComposingBuffer(true);
  auto endState = handleKeySequence(asciiKeys("jp6A"));
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "文a");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("文a"));
}

TEST_F(KeyHandlerTest,
       UppercaseLetterConvertedToLowercaseIfComposingBufferIsEmpty) {
  keyHandler_->setPutLowercaseLettersToComposingBuffer(true);
  auto endState = handleKeySequence(asciiKeys("A"));
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "a");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("a"));
}

TEST_F(KeyHandlerTest, ToneMarkOnlyStaysInReadingState) {
  auto keys = asciiKeys("6");
  keys.emplace_back(Key::namedKey(Key::KeyName::HOME));
  auto endState = handleKeySequence(keys, /*expectHandled=*/true,
                                    /*expectErrorCallbackAtEnd=*/true);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ˊ");
  // Cursor must not move.
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ˊ"));
}

TEST_F(KeyHandlerTest, ToneMarkAndToneMarkStaysInReadingState) {
  auto keys = asciiKeys("63");
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ˇ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ˇ"));
}

TEST_F(KeyHandlerTest, ToneMarkOnlyRequiresExtraSpaceToCompose) {
  auto keys = asciiKeys("6 ");
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ˊ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("ˊ"));
}

TEST_F(KeyHandlerTest,
       ToneMarkThenNonToneComponentResultingInCompositionCase1) {
  auto keys = asciiKeys("6u");
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "一");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("一"));
}

TEST_F(KeyHandlerTest,
       ToneMarkThenNonToneComponentResultingInCompositionCase2) {
  auto keys = asciiKeys("6u3");
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "一ˇ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("一ˇ"));
}

TEST_F(KeyHandlerTest,
       ToneMarkThenNonToneComponentResultingInCompositionCase3) {
  auto keys = asciiKeys("3u3");
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "以ˇ");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("以ˇ"));
}

TEST_F(KeyHandlerTest,
       ToneMarkThenNonToneComponentResultingInCompositionCase4) {
  // The last space key composes a ChoosingCandidate.
  auto keys = asciiKeys("3u ");
  auto endState = handleKeySequence(keys);
  auto inputtingState =
      dynamic_cast<InputStates::ChoosingCandidate*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "以");
  ASSERT_EQ(inputtingState->cursorIndex, strlen("以"));
}

TEST_F(
    KeyHandlerTest,
    NonViableCompositionShouldRevertToEmptyStateIfComposingBufferEndsUpEmptyCase1) {
  auto keys = asciiKeys("13");
  // ㄅˇ is not a viable composition.
  auto endState = handleKeySequence(keys, /*expectHandled=*/true,
                                    /*expectErrorCallbackAtEnd=*/true);
  auto emptyState = dynamic_cast<InputStates::Empty*>(endState.get());
  ASSERT_TRUE(emptyState != nullptr);
}

TEST_F(
    KeyHandlerTest,
    NonViableCompositionShouldRevertToEmptyStateIfComposingBufferEndsUpEmptyCase2) {
  auto keys = asciiKeys("313");
  // "ˇㄅ" is not valid in McBopomofo. We are tolerant for some cases, such as
  // we accept "ˇ一"  to be "以" since it is usually a user just want to type
  // "一ˇ". However, typing "ˇㄅ" does not make sense.
  auto endState = handleKeySequence(keys);
  auto inputtingState = dynamic_cast<InputStates::Inputting*>(endState.get());
  ASSERT_TRUE(inputtingState != nullptr);
  ASSERT_EQ(inputtingState->composingBuffer, "ˇ");
}

TEST_F(
    KeyHandlerTest,
    NonViableCompositionShouldRevertToEmptyStateIfComposingBufferEndsUpEmptyCase3) {
  auto keys = asciiKeys("31 ");
  // "ˇㄅ" is not valid in McBopomofo. We are tolerant for some cases, such as
  // we accept "ˇ一"  to be "以" since it is usually a user just want to type
  // "一ˇ". However, typing "ˇㄅ" does not make sense.
  auto endState = handleKeySequence(keys, /*expectHandled=*/false,
                                    /*expectErrorCallbackAtEnd=*/false);
  auto emptyState = dynamic_cast<InputStates::Empty*>(endState.get());
  ASSERT_TRUE(emptyState != nullptr);
}

}  // namespace McBopomofo
