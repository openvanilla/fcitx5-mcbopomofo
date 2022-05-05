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

#include "LanguageModelLoader.h"

#include <fcitx-utils/standardpath.h>

#include <filesystem>
#include <fstream>
#include <memory>

#include "Log.h"

namespace McBopomofo {

constexpr char kDataPath[] = "data/mcbopomofo-data.txt";
constexpr char kUserPhraseFilename[] = "data.txt";  // same as macOS version
constexpr char kExcludedPhraseFilename[] = "exclude-phrases.txt";  // ditto

LanguageModelLoader::LanguageModelLoader()
    : lm_(std::make_shared<McBopomofoLM>()) {
  std::string buildInLMPath = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kDataPath);
  FCITX_MCBOPOMOFO_INFO() << "Built-in LM: " << buildInLMPath;
  lm_->loadLanguageModel(buildInLMPath.c_str());
  if (!lm_->isDataModelLoaded()) {
    FCITX_MCBOPOMOFO_INFO() << "Failed to open built-in LM";
  }

  std::string userDataPath = fcitx::StandardPath::global().userDirectory(
      fcitx::StandardPath::Type::PkgData);

  // fcitx5 is configured to not to provide userDataPath, bail.
  if (userDataPath.empty()) {
    return;
  }

  userDataPath += "/mcbopomofo";
  if (!std::filesystem::exists(userDataPath)) {
    bool result = std::filesystem::create_directory(userDataPath);
    if (result) {
      FCITX_MCBOPOMOFO_INFO()
          << "Created user data directory: " << userDataPath;
    } else {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to create user data directory: " << userDataPath;
      return;
    }
  }

  // We just use very simple file handling routines.
  userDataPath_ = userDataPath;
  userPhrasesPath_ = userDataPath + "/" + kUserPhraseFilename;
  excludedPhrasesPath_ = userDataPath + "/" + kExcludedPhraseFilename;
  populateUserDataFilesIfNeeded();
  reloadUserModelsIfNeeded();
}

void LanguageModelLoader::addUserPhrase(const std::string_view& reading,
                                        const std::string_view& phrase) {
  if (userPhrasesPath_.empty() || !std::filesystem::exists(userPhrasesPath_)) {
    FCITX_MCBOPOMOFO_INFO()
        << "Not writing user phrases: data file does not exist";
    return;
  }

  // TODO(unassigned): Guard against the case where the last byte is not "\n"?
  std::ofstream ofs(userPhrasesPath_, std::ios_base::app);
  ofs << phrase << " " << reading << "\n";
  ofs.close();

  FCITX_MCBOPOMOFO_INFO() << "Added user phrase: " << phrase
                          << ", reading: " << reading;
  reloadUserModelsIfNeeded();
}

void LanguageModelLoader::reloadUserModelsIfNeeded() {
  bool shouldReload = false;
  const char* userPhrasesPathPtr = nullptr;
  const char* excludedPhrasesPathPtr = nullptr;

  if (!userPhrasesPath_.empty() && std::filesystem::exists(userPhrasesPath_)) {
    std::filesystem::file_time_type t =
        std::filesystem::last_write_time(userPhrasesPath_);
    if (t != userPhrasesTimestamp_) {
      shouldReload = true;
      userPhrasesTimestamp_ = t;
      FCITX_MCBOPOMOFO_INFO() << "Will load: " << userPhrasesPath_;
    }
    userPhrasesPathPtr = userPhrasesPath_.c_str();
  }

  if (!excludedPhrasesPath_.empty() &&
      std::filesystem::exists(excludedPhrasesPath_)) {
    std::filesystem::file_time_type t =
        std::filesystem::last_write_time(excludedPhrasesPath_);
    if (t != excludedPhrasesTimestamp_) {
      shouldReload = true;
      excludedPhrasesTimestamp_ = t;
      FCITX_MCBOPOMOFO_INFO() << "Will load: " << excludedPhrasesPath_;
    }
    excludedPhrasesPathPtr = excludedPhrasesPath_.c_str();
  }

  if (shouldReload) {
    lm_->loadUserPhrases(userPhrasesPathPtr, excludedPhrasesPathPtr);
  }
}

void LanguageModelLoader::populateUserDataFilesIfNeeded() {
  if (!userPhrasesPath_.empty() && !std::filesystem::exists(userPhrasesPath_)) {
    std::ofstream ofs(userPhrasesPath_);
    if (ofs) {
      FCITX_MCBOPOMOFO_INFO() << "Creating: " << userPhrasesPath_;

      // TODO(unassigned): Populate date here
      ofs << "# user phrases file\n";
      ofs.close();
    }
  }

  if (!excludedPhrasesPath_.empty() &&
      !std::filesystem::exists(excludedPhrasesPath_)) {
    std::ofstream ofs(excludedPhrasesPath_);
    if (ofs) {
      FCITX_MCBOPOMOFO_INFO() << "Creating: " << excludedPhrasesPath_;

      // TODO(unassigned): Populate date here
      ofs << "# excluded phrases file\n";
      ofs.close();
    }
  }
}

}  // namespace McBopomofo
