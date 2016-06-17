#include "MantidAPI/ADSValidator.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid {
namespace API {

/// Default constructor. Sets up an empty list of valid values.
ADSValidator::ADSValidator(const bool allowMultiSelection,
                           const bool isOptional)
    : TypedValidator<std::vector<std::string>>(),
      m_AllowMultiSelection(allowMultiSelection), m_isOptional(isOptional) {}

/// Clone the validator
IValidator_sptr ADSValidator::clone() const {
  return boost::make_shared<ADSValidator>(*this);
}

bool ADSValidator::isMultipleSelectionAllowed() {
  return m_AllowMultiSelection;
}

void ADSValidator::setMultipleSelectionAllowed(
    const bool isMultiSelectionAllowed) {
  m_AllowMultiSelection = isMultiSelectionAllowed;
}

bool ADSValidator::isOptional() const { return m_isOptional; }

void ADSValidator::setOptional(const bool setOptional) {
  m_isOptional = setOptional;
}

/** Checks if the string passed is in the ADS, or if all members are in the ADS
*  @param value :: The value to test
*  @return "" if the value is on the list, or "The workspace is not in the
* workspace list"
*/
std::string
ADSValidator::checkValidity(const std::vector<std::string> &value) const {
  if (!m_isOptional && (value.size() == 0))
    return "Select a value";
  if (!m_AllowMultiSelection && (value.size() > 1)) {
    return "Only one workspace was expected.";
  }
  auto &ads = AnalysisDataService::Instance();
  std::ostringstream os;
  for (std::string wsName : value) {
    if (!ads.doesExist(wsName)) {
      os << "The workspace \"" << wsName << "\" is not in the workspace list.";
    }
  }
  return os.str();
}

/** Returns the current contents of the AnalysisDataService for input
* workspaces.
*  For output workspaces, an empty set is returned
*  @return set of objects in AnalysisDataService
*/
std::vector<std::string> ADSValidator::allowedValues() const {
  // Get the list of workspaces currently in the ADS
  auto vals = AnalysisDataService::Instance().getObjectNames();
  if (isOptional()) // Insert an empty option
  {
    vals.insert("");
  }
  auto values = std::vector<std::string>(vals.begin(), vals.end());
  std::sort(values.begin(), values.end());
  return values;
}

} // namespace API
} // namespace Mantid
