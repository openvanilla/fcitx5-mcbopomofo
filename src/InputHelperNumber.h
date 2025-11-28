#ifndef MCBOPOMOFO_SRC_INPUTHELPERNUMBER_H_
#define MCBOPOMOFO_SRC_INPUTHELPERNUMBER_H_

#include <memory>
#include <string>
#include <vector>

#include "Engine/gramambular2/language_model.h"

namespace McBopomofo {
namespace InputHelperNumber {

std::vector<std::string> FillCandidatesWithNumber(
    std::string number,
    std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel);

}  // namespace InputHelperNumber
}  // namespace McBopomofo

#endif /* MCBOPOMOFO_SRC_INPUTHELPERNUMBER_H_ */
