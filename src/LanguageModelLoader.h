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

#ifndef SRC_LANGUAGEMODELLOADER_H_
#define SRC_LANGUAGEMODELLOADER_H_

#include <memory>
#include <string>
#include <string_view>

#include "Engine/McBopomofoLM.h"
#include "InputMacro.h"
#include "InputMode.h"
#include "TimestampedPath.h"

namespace McBopomofo {

class UserPhraseAdder {
 public:
  virtual ~UserPhraseAdder() = default;
  virtual void addUserPhrase(const std::string_view& reading,
                             const std::string_view& phrase) = 0;
  virtual void removeUserPhrase(const std::string_view& reading,
                                const std::string_view& phrase) = 0;
};

class LanguageModelLoader : public UserPhraseAdder {
 public:
  class LocalizedStrings;

  explicit LanguageModelLoader(
      std::unique_ptr<LocalizedStrings> localizedStrings);

  std::shared_ptr<McBopomofoLM> getLM() { return lm_; }

  void loadModelForMode(McBopomofo::InputMode mode);

  void addUserPhrase(const std::string_view& reading,
                     const std::string_view& phrase) override;

  void removeUserPhrase(const std::string_view& reading,
                        const std::string_view& phrase) override;

  void reloadUserModelsIfNeeded();

  std::string userDataPath() const { return userDataPath_; }

  std::string userPhrasesPath() const { return userPhrasesPath_.path(); }

  std::string excludedPhrasesPath() const {
    return excludedPhrasesPath_.path();
  }

 private:
  void populateUserDataFilesIfNeeded();
  bool checkIfPhraseExists(const std::filesystem::path& path,
                           const std::string& reading,
                           const std::string& value) const;
  bool addPhraseToEndOfFile(const std::filesystem::path& path,
                            const std::string& reading,
                            const std::string& value) const;
  bool removePhraseFromFile(const std::filesystem::path& path,
                            const std::string& reading,
                            const std::string& value) const;

  std::unique_ptr<LocalizedStrings> localizedStrings_;

  std::shared_ptr<McBopomofoLM> lm_;

  std::string userDataPath_;
  TimestampedPath userPhrasesPath_;
  TimestampedPath excludedPhrasesPath_;
  TimestampedPath phrasesReplacementPath_;
  InputMacroController inputMacroController_;

 public:
  class LocalizedStrings {
   public:
    virtual ~LocalizedStrings() = default;
    virtual std::string userPhraseFileHeader() = 0;
    virtual std::string excludedPhraseFileHeader() = 0;
  };
};

}  // namespace McBopomofo

#endif  // SRC_LANGUAGEMODELLOADER_H_
