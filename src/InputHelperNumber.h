#ifndef INPUTHELPERNUMBER
#define INPUTHELPERNUMBER

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

#endif /* INPUTHELPERNUMBER */
