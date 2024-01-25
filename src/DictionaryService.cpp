#include "DictionaryService.h"

#include <fcitx-utils/i18n.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/standardpath.h>
#include <fmt/format.h>
#include <json-c/json.h>

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
      auto copy =
          std::make_unique<McBopomofo::InputStates::SelectingDictionary>(
              *selecting);
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
    return;
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::unique_ptr<char[]> json_data(new char[file_size + 1]);
  fread(json_data.get(), 1, file_size, file);
  fclose(file);
  json_data[file_size] = '\0';
  struct json_object* json_obj = json_tokener_parse(json_data.get());
  if (json_obj == nullptr) {
    return;
  }
  struct json_object* servicesArray;
  if (json_object_object_get_ex(json_obj, "services", &servicesArray)) {
    array_list* list = json_object_get_array(servicesArray);
    for (size_t i = 0; i < array_list_length(list); i++) {
      auto* element = (struct json_object*)array_list_get_idx(list, i);
      struct json_object* name;
      struct json_object* url_template;
      if (json_object_object_get_ex(element, "name", &name) &&
          json_object_object_get_ex(element, "url_template", &url_template)) {
        std::string name_str = std::string(json_object_get_string(name));
        std::string url_template_str =
            std::string(json_object_get_string(url_template));
        auto service = std::make_unique<HttpBasedDictionaryService>(
            name_str, url_template_str);
        services_.push_back(std::move(service));
      }
    }
  }
  json_object_put(json_obj);
}

McBopomofo::DictionaryServices::~DictionaryServices() = default;
