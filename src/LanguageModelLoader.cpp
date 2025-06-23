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
  std::string readingStr(reading);
  std::string phraseStr(phrase);
  if (!userPhrasesPath_.pathExists()) {
    FCITX_MCBOPOMOFO_INFO()
        << "Not writing user phrases: data file does not exist";
    return;
  }
  removePhraseFromFile(excludedPhrasesPath_.path(), readingStr, phraseStr);
  if (checkIfPhraseExists(userPhrasesPath_.path(), readingStr, phraseStr)) {
    FCITX_MCBOPOMOFO_INFO()
        << "Phrase already exists: " << phrase << ", reading: " << reading;
    return;
  }
  if (!addPhraseToEndOfFile(userPhrasesPath_.path(), readingStr, phraseStr)) {
    FCITX_MCBOPOMOFO_WARN()
        << "Failed to add user phrase: " << phrase << ", reading: " << reading;
    return;
  }
  FCITX_MCBOPOMOFO_INFO() << "Added user phrase: " << phrase
                          << ", reading: " << reading;
  reloadUserModelsIfNeeded();
}

void LanguageModelLoader::removeUserPhrase(const std::string_view& reading,
                                           const std::string_view& phrase) {
  std::string readingStr(reading);
  std::string phraseStr(phrase);
  if (!excludedPhrasesPath_.pathExists()) {
    FCITX_MCBOPOMOFO_INFO()
        << "Not writing excluded phrases: data file does not exist";
    return;
  }
  removePhraseFromFile(userPhrasesPath_.path(), readingStr, phraseStr);
  if (checkIfPhraseExists(excludedPhrasesPath_.path(), readingStr, phraseStr)) {
    FCITX_MCBOPOMOFO_INFO()
        << "Phrase already excluded: " << phrase << ", reading: " << reading;
    return;
  }
  if (!addPhraseToEndOfFile(excludedPhrasesPath_.path(), readingStr,
                            phraseStr)) {
    FCITX_MCBOPOMOFO_WARN()
        << "Failed to exclude phrase: " << phrase << ", reading: " << reading;
    return;
  }
  FCITX_MCBOPOMOFO_INFO() << "Excluded phrase: " << phrase
                          << ", reading: " << reading;
  reloadUserModelsIfNeeded();
}

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

bool LanguageModelLoader::checkIfPhraseExists(const std::filesystem::path& path,
                                              const std::string& reading,
                                              const std::string& value) const {
  std::string joined = std::string(value) + " " + std::string(reading);
  std::ifstream file(path);
  if (!file.is_open()) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to open file: " << path;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Remove trailing whitespace
    size_t end = line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
      line = line.substr(0, end + 1);
    }

    if (line == joined) {
      return true;
    }
  }
  return false;
}

bool LanguageModelLoader::addPhraseToEndOfFile(
    const std::filesystem::path& path, const std::string& reading,
    const std::string& value) const {
  // First check if file exists
  if (!std::filesystem::exists(path)) {
    FCITX_MCBOPOMOFO_WARN() << "File does not exist: " << path;
    return false;
  }

  // Check if file is readable
  std::ifstream fileCheck(path);
  if (!fileCheck.is_open()) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to open file for reading: " << path;
    return false;
  }

  // Check if file ends with a newline
  bool needsNewline = false;
  fileCheck.seekg(0, std::ios::end);
  std::streampos fileSize = fileCheck.tellg();

  if (fileSize > 0) {
    fileCheck.seekg(-1, std::ios::end);
    char lastChar;
    if (fileCheck.get(lastChar)) {
      needsNewline = (lastChar != '\n');
    }
  }
  fileCheck.close();

  // Open file for appending
  std::ofstream file(path, std::ios::app);
  if (!file.is_open()) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to open file for writing: " << path;
    return false;
  }

  // Add newline if needed, then add the new phrase
  if (needsNewline) {
    file << '\n';
  }

  // Write the phrase and reading to the file
  file << value << " " << reading << '\n';

  if (!file) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to write to file: " << path;
    return false;
  }

  file.close();

  FCITX_MCBOPOMOFO_INFO() << "Added phrase to file: " << value
                          << ", reading: " << reading;
  return true;
}

bool LanguageModelLoader::removePhraseFromFile(
    const std::filesystem::path& path, const std::string& reading,
    const std::string& value) const {
  if (!std::filesystem::exists(path)) {
    FCITX_MCBOPOMOFO_WARN() << "File does not exist: " << path;
    return false;
  }

  std::string joined = value + " " + reading;
  std::ifstream inFile(path);
  if (!inFile.is_open()) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to open file for reading: " << path;
    return false;
  }

  // Create a temporary file
  std::string tempPath = path.string() + ".tmp";
  std::ofstream outFile(tempPath);
  if (!outFile.is_open()) {
    FCITX_MCBOPOMOFO_WARN() << "Failed to create temporary file: " << tempPath;
    return false;
  }

  std::string line;
  bool removed = false;
  while (std::getline(inFile, line)) {
    // Preserve comments and empty lines
    if (line.empty() || line[0] == '#') {
      outFile << line << '\n';
      continue;
    }

    // Trim trailing whitespace for comparison
    std::string trimmedLine = line;
    size_t end = trimmedLine.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
      trimmedLine = trimmedLine.substr(0, end + 1);
    }

    // Skip the line if it matches what we want to remove
    if (trimmedLine == joined) {
      removed = true;
      continue;
    }

    outFile << line << '\n';
  }

  inFile.close();
  outFile.close();

  // Replace the original file with the temporary one if something was removed
  if (removed) {
    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to rename temporary file: " << ec.message();
      std::filesystem::remove(tempPath, ec);
      return false;
    }
    FCITX_MCBOPOMOFO_INFO()
        << "Removed phrase from file: " << value << ", reading: " << reading;
    return true;
  }

  // If nothing was removed, delete the temp file
  std::filesystem::remove(tempPath);
  return false;
}

}  // namespace McBopomofo
