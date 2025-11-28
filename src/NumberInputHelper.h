#ifndef MCBOPOMOFO_SRC_NUMBERINPUTHELPER_H_
#define MCBOPOMOFO_SRC_NUMBERINPUTHELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "Engine/gramambular2/language_model.h"

namespace McBopomofo {
namespace NumberInputHelper {

std::vector<std::string> FillCandidatesWithNumber(
    std::string number,
    std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel);

}  // namespace NumberInputHelper
}  // namespace McBopomofo

#endif /* MCBOPOMOFO_SRC_NUMBERINPUTHELPER_H_ */
