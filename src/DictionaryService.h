// Copyright (c) 2024 and onwards The McBopomofo Authors.
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

#ifndef SRC_DICTIONARYSERVICE_H_
#define SRC_DICTIONARYSERVICE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "InputState.h"
#include "PathCompat.h"

namespace McBopomofo {

using StateCallback =
    std::function<void(std::unique_ptr<McBopomofo::InputState>)>;

/**
 * Represents a single dictionary service.
 */
class DictionaryService {
 public:
  virtual ~DictionaryService() = default;

  virtual std::string name() const = 0;
  virtual void lookup(std::string phrase, InputState* state,
                      size_t serviceIndex,
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

#endif  // SRC_DICTIONARYSERVICE_H_
