
#ifndef FCITX5_MCBOPOMOFO_DICTIONARYSERVICE_H
#define FCITX5_MCBOPOMOFO_DICTIONARYSERVICE_H

#include <functional>
#include <memory>
#include <string>

#include "InputState.h"

namespace McBopomofo {

using StateCallback =
    std::function<void(std::unique_ptr<McBopomofo::InputState>)>;

/**
 * Represents a single dictionary service.
 */
class DictionaryService {
 public:
  virtual ~DictionaryService() {};

  virtual std::string name() const = 0;
  virtual void lookup(std::string phrase, InputState* state, size_t serviceIndex,
                      const StateCallback& stateCallback) = 0;
  virtual std::string textForMenu(std::string selectedString) const = 0;
};

/**  Provides dictionaries that helps user to look up phrases. */
class DictionaryServices {
 public:
  DictionaryServices();
  ~DictionaryServices();
  /** Whether if there is a list of services. */
  bool hasServices();
  /** Load additional services. */
  void load();
  /**
   * Look up a phrase using the index of the service in the list.
   * @param phrase The phrase.
   * @param serviceIndex Index of the service.
   * @param state The current state.
   * @param stateCallback The state callback.
   */
  void lookup(std::string phrase, size_t serviceIndex, InputState* state,
              const StateCallback& stateCallback);
  /**
   * Create a menu by passing the selected phrase.
   * @param phrase The phrase.
   * @return The menu.
   */
  std::vector<std::string> menuForPhrase(const std::string& phrase);

 protected:
  std::vector<std::unique_ptr<DictionaryService>> services_;
};

}  // namespace McBopomofo

#endif  // FCITX5_MCBOPOMOFO_DICTIONARYSERVICE_H
