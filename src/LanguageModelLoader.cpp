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
#include <utility>

#include "Log.h"

namespace McBopomofo {

constexpr char kDataPath[] = "data/mcbopomofo-data.txt";
constexpr char kDataPathPlainBPMF[] = "data/mcbopomofo-data-plain-bpmf.txt";
constexpr char kUserPhraseFilename[] = "data.txt";  // same as macOS version
constexpr char kExcludedPhraseFilename[] = "exclude-phrases.txt";  // ditto
constexpr char kAssociatedPhrasesV2Path[] =
    "data/mcbopomofo-associated-phrases-v2.txt";
constexpr char kPhrasesReplacementFilename[] = "phrases-replacement.txt";

LanguageModelLoader::LanguageModelLoader(
    std::unique_ptr<LocalizedStrings> localizedStrings)
    : localizedStrings_(std::move(localizedStrings)),
      lm_(std::make_shared<McBopomofoLM>()) {
  std::string buildInLMPath = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kDataPath);
  FCITX_MCBOPOMOFO_INFO() << "Built-in LM: " << buildInLMPath;
  lm_->loadLanguageModel(buildInLMPath.c_str());
  if (!lm_->isDataModelLoaded()) {
    FCITX_MCBOPOMOFO_INFO() << "Failed to open built-in LM";
  }

  // Load associated phrases v2.
  std::string associatedPhrasesV2Path = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kAssociatedPhrasesV2Path);
  FCITX_MCBOPOMOFO_INFO() << "Associated phrases: " << associatedPhrasesV2Path;
  lm_->loadAssociatedPhrasesV2(associatedPhrasesV2Path.c_str());

  FCITX_MCBOPOMOFO_INFO() << "Set macro converter";
  auto converter = [this](const std::string& input) {
    return this->inputMacroController_.handle(input);
  };
  lm_->setMacroConverter(converter);

  std::string userDataPath = fcitx::StandardPath::global().userDirectory(
      fcitx::StandardPath::Type::PkgData);

  // fcitx5 is configured not to provide userDataPath, bail.
  if (userDataPath.empty()) {
    return;
  }

  [[maybe_unused]] std::error_code err;
  if (!std::filesystem::exists(userDataPath, err)) {
    bool result = std::filesystem::create_directory(userDataPath, err);
    if (result) {
      FCITX_MCBOPOMOFO_INFO()
          << "Created fcitx5 user data directory: " << userDataPath;
    } else {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to create fcitx5 user data directory: " << userDataPath;
      return;
    }
  }

  userDataPath += "/mcbopomofo";
  if (!std::filesystem::exists(userDataPath), err) {
    bool result = std::filesystem::create_directory(userDataPath, err);
    if (result) {
      FCITX_MCBOPOMOFO_INFO()
          << "Created mcbopomofo user data directory: " << userDataPath;
    } else {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to create mcbopomofo user data directory: "
          << userDataPath;
      return;
    }
  }

  // We just use very simple file handling routines.
  userDataPath_ = userDataPath;
  userPhrasesPath_ = TimestampedPath(userDataPath + "/" + kUserPhraseFilename);
  excludedPhrasesPath_ =
      TimestampedPath(userDataPath + "/" + kExcludedPhraseFilename);
  phrasesReplacementPath_ =
      TimestampedPath(userDataPath + "/" + kPhrasesReplacementFilename);
  populateUserDataFilesIfNeeded();
  reloadUserModelsIfNeeded();
}

void LanguageModelLoader::loadModelForMode(McBopomofo::InputMode mode) {
  const char* path = mode == McBopomofo::InputMode::PlainBopomofo
                         ? kDataPathPlainBPMF
                         : kDataPath;

  std::string buildInLMPath = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, path);

  FCITX_MCBOPOMOFO_INFO() << "Built-in LM: " << buildInLMPath;
  lm_->loadLanguageModel(buildInLMPath.c_str());
  if (!lm_->isDataModelLoaded()) {
    FCITX_MCBOPOMOFO_INFO() << "Failed to open built-in LM";
    return;
  }
}

void LanguageModelLoader::addUserPhrase(const std::string_view& reading,
                                        const std::string_view& phrase) {
  if (!userPhrasesPath_.pathExists()) {
    FCITX_MCBOPOMOFO_INFO()
        << "Not writing user phrases: data file does not exist";
    return;
  }

  // TODO(unassigned): Guard against the case where the last byte is not "\n"?
  std::ofstream ofs(userPhrasesPath_.path(), std::ios_base::app);
  ofs << phrase << " " << reading << "\n";
  ofs.close();

  FCITX_MCBOPOMOFO_INFO() << "Added user phrase: " << phrase
                          << ", reading: " << reading;
  reloadUserModelsIfNeeded();
}

void LanguageModelLoader::removeUserPhrase(const std::string_view& reading,
                                           const std::string_view& phrase) {}

void LanguageModelLoader::reloadUserModelsIfNeeded() {
  bool shouldReloadUserPhrases = false;
  bool shouldReloadPhrasesReplacement = false;

  if (userPhrasesPath_.pathExists() &&
      userPhrasesPath_.timestampDifferentFromLastCheck()) {
    shouldReloadUserPhrases = true;
    userPhrasesPath_.checkTimestamp();
    FCITX_MCBOPOMOFO_INFO() << "Will load: " << userPhrasesPath_.path();
  }

  if (excludedPhrasesPath_.pathExists() &&
      excludedPhrasesPath_.timestampDifferentFromLastCheck()) {
    shouldReloadUserPhrases = true;
    excludedPhrasesPath_.checkTimestamp();
    FCITX_MCBOPOMOFO_INFO() << "Will load: " << excludedPhrasesPath_.path();
  }

  // Phrases replacement is considered an advanced feature. We only enable
  // it and check for updates if the file exists. If the file disappears,
  // disable it.
  if (!phrasesReplacementPath_.path().empty()) {
    bool isEnabled = lm_->phraseReplacementEnabled();
    bool fileExists = phrasesReplacementPath_.pathExists();

    if (isEnabled && !fileExists) {
      // Disable phrases replacement now that the file is gone.
      lm_->setPhraseReplacementEnabled(false);
      // Reset the timestamp.
      phrasesReplacementPath_.checkTimestamp();
      FCITX_MCBOPOMOFO_INFO() << "Phrases replacement disabled, file gone: "
                              << phrasesReplacementPath_.path();
    } else if (fileExists) {
      if (phrasesReplacementPath_.timestampDifferentFromLastCheck()) {
        shouldReloadPhrasesReplacement = true;
        phrasesReplacementPath_.checkTimestamp();
      }

      if (shouldReloadPhrasesReplacement) {
        if (isEnabled) {
          FCITX_MCBOPOMOFO_INFO() << "Will reload phrases replacement file: "
                                  << phrasesReplacementPath_.path();
        } else {
          lm_->setPhraseReplacementEnabled(true);
          FCITX_MCBOPOMOFO_INFO() << "Phrases replacement enabled, file: "
                                  << phrasesReplacementPath_.path();
        }
      }
    }
  }

  if (shouldReloadUserPhrases) {
    lm_->loadUserPhrases(userPhrasesPath_.path().c_str(),
                         excludedPhrasesPath_.path().c_str());
  }

  if (shouldReloadPhrasesReplacement) {
    lm_->loadPhraseReplacementMap(phrasesReplacementPath_.path().c_str());
  }
}

void LanguageModelLoader::populateUserDataFilesIfNeeded() {
  if (!userPhrasesPath_.path().empty() && !userPhrasesPath_.pathExists()) {
    std::ofstream ofs(userPhrasesPath_.path());
    if (ofs) {
      FCITX_MCBOPOMOFO_INFO() << "Creating: " << userPhrasesPath_.path();
      ofs << localizedStrings_->userPhraseFileHeader();
      ofs.close();
    }
  }

  if (!excludedPhrasesPath_.path().empty() &&
      !excludedPhrasesPath_.pathExists()) {
    std::ofstream ofs(excludedPhrasesPath_.path());
    if (ofs) {
      FCITX_MCBOPOMOFO_INFO() << "Creating: " << excludedPhrasesPath_.path();
      ofs << localizedStrings_->excludedPhraseFileHeader();
      ofs.close();
    }
  }
}

}  // namespace McBopomofo
