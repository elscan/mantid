// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"

#include "MantidCurveFitting/Algorithms/IqtFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidCurveFitting/Algorithms/QENSFitSimultaneous.h"

#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");

std::string getPropertySuffix(std::size_t index) { return index == 0 ? "" : "_" + std::to_string(index); }
} // namespace

namespace Mantid::CurveFitting::Algorithms {

using namespace API;

/// Algorithms name for identification. @see Algorithm::name
template <> const std::string IqtFit<QENSFitSequential>::name() const { return "IqtFitSequential"; }

template <> const std::string IqtFit<QENSFitSimultaneous>::name() const { return "IqtFitSimultaneous"; }

template <typename Base> const std::string IqtFit<Base>::name() const { return "IqtFit"; }

/// Algorithm's version for identification. @see Algorithm::version
template <typename Base> int IqtFit<Base>::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
template <typename Base> const std::string IqtFit<Base>::category() const { return "Workflow\\MIDAS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
template <> const std::string IqtFit<QENSFitSequential>::summary() const {
  return "Fits data files (\\*\\_iqt) generated by I(Q, t) sequentially.";
}

template <> const std::string IqtFit<QENSFitSimultaneous>::summary() const {
  return "Fits data files (\\*\\_iqt) generated by I(Q, t) simultaneously.";
}

template <typename Base> const std::string IqtFit<Base>::summary() const {
  return "Fits an \\*\\_iqt file generated by I(Q, t) sequentially.";
}

/// Algorithm's see also for related algorithms. @see Algorithm::seeAlso
template <> const std::vector<std::string> IqtFit<QENSFitSequential>::seeAlso() const { return {"QENSFitSequential"}; }

template <> const std::vector<std::string> IqtFit<QENSFitSimultaneous>::seeAlso() const {
  return {"QENSFitSimultaneous"};
}

template <typename Base> const std::vector<std::string> IqtFit<Base>::seeAlso() const { return {}; }

template <> std::map<std::string, std::string> IqtFit<QENSFitSequential>::validateInputs() {
  auto errors = QENSFitSequential::validateInputs();
  const std::vector<double> startX = QENSFitSequential::getProperty("StartX");
  for (const double &start : startX) {
    if (start < 0) {
      errors["StartX"] = "StartX must be greater than or equal to 0.";
    }
  }
  return errors;
}

template <typename Base> std::map<std::string, std::string> IqtFit<Base>::validateInputs() {
  return Base::validateInputs();
}

template <typename Base> bool IqtFit<Base>::isFitParameter(const std::string &name) const {
  return name.rfind("A0") != std::string::npos || name.rfind("Height") != std::string::npos ||
         name.rfind("Stretching") != std::string::npos || name.rfind("Lifetime") != std::string::npos;
}

template <typename Base> bool IqtFit<Base>::throwIfElasticQConversionFails() const { return true; }

// Register the algorithms into the AlgorithmFactory
template class IqtFit<QENSFitSequential>;
template class IqtFit<QENSFitSimultaneous>;

using IqtFitSequential = IqtFit<QENSFitSequential>;
using IqtFitSimultaneous = IqtFit<QENSFitSimultaneous>;

DECLARE_ALGORITHM(IqtFitSequential)
DECLARE_ALGORITHM(IqtFitSimultaneous)

} // namespace Mantid::CurveFitting::Algorithms
