#include "DictionaryService.h"

#include <cjson/cJSON.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/standardpath.h>
#include <fmt/format.h>

#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

#include "Log.h"

constexpr char kDataPath[] = "data/mcbopomofo-dictionary-service.json";

std::string urlEncode(const std::string& str) {
  std::ostringstream encodedStream;
  encodedStream << std::hex << std::uppercase << std::setfill('0');

  for (char c : str) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedStream << c;
    } else {
      encodedStream << '%' << std::setw(2)
                    << static_cast<unsigned int>(static_cast<unsigned char>(c));
    }
  }

  return encodedStream.str();
}

class CharacterInfoService : public McBopomofo::DictionaryService {
 public:
  std::string name() const override {
    return fmt::format(_("Character Information"));
  }

  void lookup(std::string phrase, McBopomofo::InputState* state,
              size_t /*Unused*/,
              const McBopomofo::StateCallback& stateCallback) override {
    auto selecting =
        dynamic_cast<McBopomofo::InputStates::SelectingDictionary*>(state);
    if (selecting != nullptr) {
      auto copy = selecting->copy();
      auto newState =
          std::make_unique<McBopomofo::InputStates::ShowingCharInfo>(
              std::move(copy), phrase);
      stateCallback(std::move(newState));
    }
  }

  std::string textForMenu(std::string /*Unused*/) const override {
    return fmt::format(_("Character Information"));
  }
};

class HttpBasedDictionaryService : public McBopomofo::DictionaryService {
 public:
  HttpBasedDictionaryService(std::string name, std::string urlTemplate)
      : name_(std::move(name)), urlTemplate_(std::move(urlTemplate)) {}
  ~HttpBasedDictionaryService() override = default;
  std::string name() const override { return name_; };

  void lookup(std::string phrase, McBopomofo::InputState* /*unused*/,
              size_t /*unused*/,
              const McBopomofo::StateCallback& /*unused*/) override {
    std::string url = urlTemplate_;
    std::string encoded = "(encoded)";
    url.replace(url.find(encoded), url.length(), urlEncode(phrase));
    fcitx::startProcess({"xdg-open", url});
  }

  std::string textForMenu(std::string selectedString) const override {
    return fmt::format(_("Look up \"{0}\" in {1}"), selectedString, name_);
  }

 private:
  std::string name_;
  std::string urlTemplate_;
};

McBopomofo::DictionaryServices::DictionaryServices() = default;

bool McBopomofo::DictionaryServices::hasServices() {
  return !services_.empty();
}

void McBopomofo::DictionaryServices::lookup(
    std::string phrase, size_t serviceIndex, InputState* state,
    const StateCallback& stateCallback) {
  if (serviceIndex >= services_.size()) {
    return;
  }
  auto service = services_[serviceIndex].get();
  service->lookup(std::move(phrase), state, serviceIndex, stateCallback);
}

std::vector<std::string> McBopomofo::DictionaryServices::menuForPhrase(
    const std::string& phrase) {
  std::vector<std::string> menu;
  for (const auto& service : services_) {
    std::string item = service->textForMenu(phrase);
    menu.emplace_back(item);
  }
  return menu;
}

void McBopomofo::DictionaryServices::load() {
  services_.emplace_back(std::make_unique<CharacterInfoService>());

  // Load json and add to services_
  std::string dictionaryServicesPath = fcitx::StandardPath::global().locate(
      fcitx::StandardPath::Type::PkgData, kDataPath);
  FILE* file = fopen(dictionaryServicesPath.c_str(), "r");
  if (!file) {
    FCITX_MCBOPOMOFO_INFO()
        << "No dictionary service file" << dictionaryServicesPath;
    //    fclose(file);
    return;
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::unique_ptr<char[]> json_data(new char[file_size + 1]);
  fread(json_data.get(), 1, file_size, file);
  fclose(file);
  json_data[file_size] = '\0';
  std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(
      cJSON_Parse(json_data.get()), &cJSON_Delete);
  if (!root) {
    return;
  }

  cJSON* servicesArray = cJSON_GetObjectItem(root.get(), "services");
  if (!servicesArray || !cJSON_IsArray(servicesArray)) {
    return;
  }

  int arraySize = cJSON_GetArraySize(servicesArray);
  for (int i = 0; i < arraySize; ++i) {
    cJSON* serviceObject = cJSON_GetArrayItem(servicesArray, i);
    if (!serviceObject || !cJSON_IsObject(serviceObject)) {
      continue;
    }

    cJSON* nameObject = cJSON_GetObjectItem(serviceObject, "name");
    cJSON* urlTemplateObject =
        cJSON_GetObjectItem(serviceObject, "url_template");

    if (nameObject && urlTemplateObject && cJSON_IsString(nameObject) &&
        cJSON_IsString(urlTemplateObject)) {
      std::unique_ptr<DictionaryService> service =
          std::make_unique<HttpBasedDictionaryService>(
              std::string(cJSON_GetStringValue(nameObject)),
              std::string(cJSON_GetStringValue(urlTemplateObject)));
      services_.push_back(std::move(service));
    }
  }
}

McBopomofo::DictionaryServices::~DictionaryServices() = default;
