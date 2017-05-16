#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

namespace Mantid {
namespace Algorithms {

/*Anonomous namespace */
namespace {

/** Get the twoTheta angle for the centre of the detector associated with the
* given spectrum
*
* @param spectrumInfo : the spectrum info
* @param spectrumIdx : the workspace index of the spectrum
* @return : the twoTheta angle in radians
*/
double getDetectorTwoTheta(const SpectrumInfo *spectrumInfo,
                           const size_t spectrumIdx) {
  return spectrumInfo->signedTwoTheta(spectrumIdx);
}

/** Get the twoTheta angle range for the top/bottom of the detector associated
* with the given spectrum
*
* @param spectrumInfo : the spectrum info
* @param spectrumIdx : the workspace index of the spectrum
* @return : the twoTheta angle in radians
*/
double getDetectorTwoThetaRange(const SpectrumInfo *spectrumInfo,
                                const size_t spectrumIdx) {
  // Assume the range covered by this pixel is the diff between this
  // pixel's twoTheta and the next/prev pixel)
  double twoTheta = getDetectorTwoTheta(spectrumInfo, spectrumIdx);
  double bTwoTheta = 0;

  if (spectrumIdx + 1 < spectrumInfo->size()) {
    bTwoTheta = getDetectorTwoTheta(spectrumInfo, spectrumIdx + 1) - twoTheta;
  }

  return bTwoTheta;
}

/** Get the start/end of the lambda range for the detector associated
* with the given spectrum
*
* @return : the lambda range
*/
double getLambdaRange(const HistogramX &xValues, const int xIdx) {
  // The lambda range is the bin width from the given index to the next.
  if (xIdx < 0 || xIdx + 1 >= static_cast<int>(xValues.size())) {
    throw std::runtime_error("Error accessing X values out of range (index=" +
                             std::to_string(xIdx + 1) + ", size=" +
                             std::to_string(xValues.size()));
  }

  double result = xValues[xIdx + 1] - xValues[xIdx];
  return result;
}

/** Get the lambda value at the centre of the detector associated
* with the given spectrum
*
* @return : the lambda range
*/
double getLambda(const HistogramX &xValues, const int xIdx) {
  if (xIdx < 0 || xIdx >= static_cast<int>(xValues.size())) {
    throw std::runtime_error("Error accessing X values out of range (index=" +
                             std::to_string(xIdx) + ", size=" +
                             std::to_string(xValues.size()));
  }

  // The centre of the bin is the lower bin edge plus half the width
  return xValues[xIdx] + getLambdaRange(xValues, xIdx) / 2.0;
}

/** @todo The following translate functions are duplicates of code in
* GroupDetectors2.cpp. We should move them to a common location if possible */

/* The following functions are used to translate single operators into
* groups, just like the ones this algorithm loads from .map files.
*
* Each function takes a string, such as "3+4", or "6:10" and then adds
* the resulting groups of spectra to outGroups.
*/

// An add operation, i.e. "3+4" -> [3+4]
void translateAdd(const std::string &instructions,
                  std::vector<std::vector<size_t>> &outGroups) {
  auto spectra = Kernel::StringTokenizer(
      instructions, "+", Kernel::StringTokenizer::TOK_TRIM |
                             Kernel::StringTokenizer::TOK_IGNORE_EMPTY);

  std::vector<size_t> outSpectra;
  outSpectra.reserve(spectra.count());
  for (auto spectrum : spectra) {
    // add this spectrum to the group we're about to add
    outSpectra.push_back(boost::lexical_cast<size_t>(spectrum));
  }
  outGroups.push_back(std::move(outSpectra));
}

// A range summation, i.e. "3-6" -> [3+4+5+6]
void translateSumRange(const std::string &instructions,
                       std::vector<std::vector<size_t>> &outGroups) {
  // add a group with the sum of the spectra in the range
  auto spectra = Kernel::StringTokenizer(instructions, "-");
  if (spectra.count() != 2)
    throw std::runtime_error("Malformed range (-) operation.");
  // fetch the start and stop spectra
  size_t first = boost::lexical_cast<size_t>(spectra[0]);
  size_t last = boost::lexical_cast<size_t>(spectra[1]);
  // swap if they're back to front
  if (first > last)
    std::swap(first, last);

  // add all the spectra in the range to the output group
  std::vector<size_t> outSpectra;
  outSpectra.reserve(last - first + 1);
  for (size_t i = first; i <= last; ++i)
    outSpectra.push_back(i);
  if (!outSpectra.empty())
    outGroups.push_back(std::move(outSpectra));
}

// A range insertion, i.e. "3:6" -> [3,4,5,6]
void translateRange(const std::string &instructions,
                    std::vector<std::vector<size_t>> &outGroups) {
  // add a group per spectra
  auto spectra = Kernel::StringTokenizer(
      instructions, ":", Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  if (spectra.count() != 2)
    throw std::runtime_error("Malformed range (:) operation.");
  // fetch the start and stop spectra
  size_t first = boost::lexical_cast<size_t>(spectra[0]);
  size_t last = boost::lexical_cast<size_t>(spectra[1]);
  // swap if they're back to front
  if (first > last)
    std::swap(first, last);

  // add all the spectra in the range to separate output groups
  for (size_t i = first; i <= last; ++i) {
    // create group of size 1 with the spectrum and add it to output
    outGroups.emplace_back(1, i);
  }
}

/**
* Translate the processing instructions into a vector of groups of indices
*
* @param instructions : Instructions to translate
* @return : A vector of groups, each group being a vector of its 0-based
* spectrum indices
*/
std::vector<std::vector<size_t>>
translateInstructions(const std::string &instructions) {
  std::vector<std::vector<size_t>> outGroups;

  try {
    // split into comma separated groups, each group potentially containing
    // an operation (+-:) that produces even more groups.
    auto groups = Kernel::StringTokenizer(
        instructions, ",",
        StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
    for (const auto &groupStr : groups) {
      // Look for the various operators in the string. If one is found then
      // do the necessary translation into groupings.
      if (groupStr.find('+') != std::string::npos) {
        // add a group with the given spectra
        translateAdd(groupStr, outGroups);
      } else if (groupStr.find('-') != std::string::npos) {
        translateSumRange(groupStr, outGroups);
      } else if (groupStr.find(':') != std::string::npos) {
        translateRange(groupStr, outGroups);
      } else if (!groupStr.empty()) {
        // contains no instructions, just add this spectrum as a new group
        // create group of size 1 with the spectrum in it and add it to output
        outGroups.emplace_back(1, boost::lexical_cast<size_t>(groupStr));
      }
    }
  } catch (boost::bad_lexical_cast &) {
    throw std::runtime_error("Invalid processing instructions: " +
                             instructions);
  }

  return outGroups;
}

/**
* Map a spectrum index from the given map to the given workspace
* @param map : the original spectrum number to index map
* @param mapIdx : the index to look up in the map
* @param destWS : the destination workspace
*/
int mapSpectrumIndexToWorkspace(const spec2index_map &map, const size_t mapIdx,
                                MatrixWorkspace_const_sptr destWS) {

  // Get the spectrum numbers for these indices
  auto it = std::find_if(map.begin(), map.end(),
                         [mapIdx](const std::pair<specnum_t, size_t> &mapIter) {
                           return mapIter.second == mapIdx;
                         });

  if (it == map.end()) {
    throw std::runtime_error("Workspace index " + std::to_string(mapIdx) +
                             " not found in run workspace ");
  }
  specnum_t specId = it->first;

  // Get the workspace index in the destination workspace
  int wsIdx = static_cast<int>(destWS->getIndexFromSpectrumNumber(specId));

  return wsIdx;
}

/**
* @param originWS : Origin workspace, which provides the original workspace
* index to spectrum number mapping.
* @param hostWS : Workspace onto which the resulting workspace indexes will be
* hosted
* @throws :: If the specId are not found to exist on the host end-point
*workspace.
* @return :: Remapped workspace indexes applicable for the host workspace,
*as a vector of groups of vectors of spectrum indices
*/
std::vector<std::vector<size_t>> mapSpectrumIndicesToWorkspace(
    MatrixWorkspace_const_sptr originWS, MatrixWorkspace_const_sptr hostWS,
    const std::vector<std::vector<size_t>> &detectorGroups) {

  auto map = originWS->getSpectrumToWorkspaceIndexMap();
  std::vector<std::vector<size_t>> hostGroups;

  for (auto group : detectorGroups) {
    std::vector<size_t> hostDetectors;
    for (auto i : group) {
      const int hostIdx = mapSpectrumIndexToWorkspace(map, i, hostWS);
      hostDetectors.push_back(hostIdx);
    }
    hostGroups.push_back(hostDetectors);
  }

  return hostGroups;
}

/**
* Translate all the workspace indexes in an origin workspace into workspace
* indexes of a host end-point workspace. This is done using spectrum numbers as
* the intermediate.
*
* @param originWS : Origin workspace, which provides the original workspace
* index to spectrum number mapping.
* @param hostWS : Workspace onto which the resulting workspace indexes will be
* hosted
* @throws :: If the specId are not found to exist on the host end-point
*workspace.
* @return :: Remapped workspace indexes applicable for the host workspace,
*as comma separated string.
*/
std::string createProcessingCommandsFromDetectorWS(
    MatrixWorkspace_const_sptr originWS, MatrixWorkspace_const_sptr hostWS,
    const std::vector<std::vector<size_t>> &detectorGroups) {

  std::string result;

  // Map the original indices to the host workspace
  std::vector<std::vector<size_t>> hostGroups =
      mapSpectrumIndicesToWorkspace(originWS, hostWS, detectorGroups);

  // Add each group to the output, separated by ','
  /// @todo Add support to separate contiguous groups by ':' to avoid having
  /// long lists in the processing instructions
  for (auto groupIt = hostGroups.begin(); groupIt != hostGroups.end();
       ++groupIt) {
    const auto &hostDetectors = *groupIt;

    // Add each detector index to the output string separated by '+' to indicate
    // that all detectors in this group will be summed. We also check for
    // contiguous ranges so we output e.g. 3-5 instead of 3+4+5
    bool contiguous = false;
    size_t contiguousStart = 0;

    for (auto it = hostDetectors.begin(); it != hostDetectors.end(); ++it) {
      // Check if the next iterator is a contiguous increment from this one
      auto nextIt = it + 1;
      if (nextIt != hostDetectors.end() && *nextIt == *it + 1) {
        // If this is a start of a new contiguous region, remember the start
        // index
        if (!contiguous) {
          contiguousStart = *it;
          contiguous = true;
        }
        // Continue to find the end of the contiguous region
        continue;
      }

      if (contiguous) {
        // Output the contiguous range, then reset the flag
        result.append(std::to_string(contiguousStart))
            .append("-")
            .append(std::to_string(*it));
        contiguousStart = 0;
        contiguous = false;
      } else {
        // Just output the value
        result.append(std::to_string(*it));
      }

      // Add a separator ready for the next value/range
      if (nextIt != hostDetectors.end()) {
        result.append("+");
      }
    }

    if (groupIt + 1 != hostGroups.end()) {
      result.append(",");
    }
  }

  return result;
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOne2)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void ReflectometryReductionOne2::init() {

  // Input workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Run to reduce.");

  initReductionProperties();

  // ThetaIn
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ThetaIn", Mantid::EMPTY_DBL(), Direction::Input),
                  "Angle in degrees");

  // Processing instructions
  declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "",
                      boost::make_shared<MandatoryValidator<std::string>>(),
                      Direction::Input),
                  "Grouping pattern on workspace indexes to yield only "
                  "the detectors of interest. See GroupDetectors for details.");

  // Minimum wavelength
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMin", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength minimum in angstroms");

  // Maximum wavelength
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMax", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength maximum in angstroms");

  // Properties for direct beam normalization
  initDirectBeamProperties();

  // Init properties for monitors
  initMonitorProperties();
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

  // Init properties for transmission normalization
  initTransmissionProperties();

  // Init properties for algorithmic corrections
  initAlgorithmicProperties();

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output Workspace IvsQ.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceWavelength",
                                                   "", Direction::Output,
                                                   PropertyMode::Optional),
                  "Output Workspace IvsLam. Intermediate workspace.");
}

/** Validate inputs
*/
std::map<std::string, std::string>
ReflectometryReductionOne2::validateInputs() {

  std::map<std::string, std::string> results;

  const auto reduction = validateReductionProperties();
  results.insert(reduction.begin(), reduction.end());

  const auto wavelength = validateWavelengthRanges();
  results.insert(wavelength.begin(), wavelength.end());

  const auto directBeam = validateDirectBeamProperties();
  results.insert(directBeam.begin(), directBeam.end());

  const auto transmission = validateTransmissionProperties();
  results.insert(transmission.begin(), transmission.end());

  return results;
}

/** Execute the algorithm.
*/
void ReflectometryReductionOne2::exec() {
  // Get input properties
  m_runWS = getProperty("InputWorkspace");
  const auto xUnitID = m_runWS->getAxis(0)->unit()->unitID();

  // Neither TOF or Lambda? Abort.
  if ((xUnitID != "Wavelength") && (xUnitID != "TOF"))
    throw std::invalid_argument(
        "InputWorkspace must have units of TOF or Wavelength");

  m_spectrumInfo = &m_runWS->spectrumInfo();

  // Find and cache detector groups and theta0
  findDetectorGroups();
  findTheta0();

  // Check whether conversion, normalisation and summation has already been done
  m_convertUnits = true;
  m_normalise = true;
  m_sum = true;
  if (xUnitID == "Wavelength") {
    // Already converted converted to wavelength
    m_convertUnits = false;
    // Assume it's also already been normalised
    m_normalise = false;
    // Assume summation is already done if the number of histograms in the input
    // is the same as the number of detector groups (which will define the
    // number of histograms in the output)
    if (m_runWS->getNumberHistograms() == detectorGroups().size()) {
      m_sum = false;
    }
  }

  // Create the output workspace in wavelength
  MatrixWorkspace_sptr IvsLam = makeIvsLam();

  // Convert to Q
  auto IvsQ = convertToQ(IvsLam);

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
}

/**
* Creates the output 1D array in wavelength from an input 2D workspace in
* TOF. Summation is done over lambda or over lines of constant Q depending on
* the type of reduction. For the latter, the output is projected to "virtual
* lambda" at a reference angle twoThetaR.
*
* @return :: the output workspace in wavelength
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::makeIvsLam() {
  MatrixWorkspace_sptr result = m_runWS;

  if (summingInQ()) {
    // Currently only support a single input range of detectors
    /// todo Generalise this to support multiple ranges
    if (detectorGroups().size() > 1) {
      throw std::runtime_error(
          "Only a single range is supported for summing in Q");
    }
    const auto &detectors = detectorGroups()[0];
    // Convert to lambda
    if (m_convertUnits) {
      g_log.debug() << "Converting input workspace to wavelength" << std::endl;
      result = convertToWavelength(result);
    }
    // Normalise
    if (m_normalise) {
      g_log.debug() << "Normalising input workspace" << std::endl;
      result = directBeamCorrection(result);
      result = monitorCorrection(result);
      result = transOrAlgCorrection(result, false);
    }
    // Do the summation in Q
    if (m_sum) {
      g_log.debug() << "Summing in Q" << std::endl;
      result = sumInQ(result, detectors);
    }
  } else {
    // Do the summation in lambda
    if (m_sum) {
      g_log.debug() << "Summing in wavelength" << std::endl;
      result = makeDetectorWS(result, m_convertUnits);
    }
    // Normalise the 1D result
    if (m_normalise) {
      g_log.debug() << "Normalising output workspace" << std::endl;
      result = directBeamCorrection(result);
      result = monitorCorrection(result);
      result = transOrAlgCorrection(result, true);
    }
    // Crop to wavelength limits
    g_log.debug() << "Cropping output workspace" << std::endl;
    result = cropWavelength(result);
  }

  return result;
}

/**
* Normalize by monitors (only if I0MonitorIndex, MonitorBackgroundWavelengthMin
* and MonitorBackgroundWavelengthMax have been given)
*
* @param detectorWS :: the detector workspace to normalise, in lambda
* @return :: the normalized workspace in lambda
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::monitorCorrection(MatrixWorkspace_sptr detectorWS) {
  MatrixWorkspace_sptr IvsLam;
  Property *monProperty = getProperty("I0MonitorIndex");
  Property *backgroundMinProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  Property *backgroundMaxProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  if (!monProperty->isDefault() && !backgroundMinProperty->isDefault() &&
      !backgroundMaxProperty->isDefault()) {
    const bool integratedMonitors =
        getProperty("NormalizeByIntegratedMonitors");
    const auto monitorWS = makeMonitorWS(m_runWS, integratedMonitors);
    if (!integratedMonitors)
      detectorWS = rebinDetectorsToMonitors(detectorWS, monitorWS);
    IvsLam = divide(detectorWS, monitorWS);
  } else {
    IvsLam = detectorWS;
  }

  return IvsLam;
}

/** Creates a direct beam workspace in wavelength from an input workspace in
* TOF. This method should only be called if RegionOfDirectBeam is provided.
*
* @param inputWS :: the input workspace in TOF
* @return :: the direct beam workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::makeDirectBeamWS(MatrixWorkspace_sptr inputWS) {

  std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
  // Sum over the direct beam.
  const std::string processingCommands = std::to_string(directBeamRegion[0]) +
                                         "-" +
                                         std::to_string(directBeamRegion[1]);

  auto groupDirectBeamAlg = this->createChildAlgorithm("GroupDetectors");
  groupDirectBeamAlg->initialize();
  groupDirectBeamAlg->setProperty("GroupingPattern", processingCommands);
  groupDirectBeamAlg->setProperty("InputWorkspace", inputWS);
  groupDirectBeamAlg->execute();
  MatrixWorkspace_sptr directBeamWS =
      groupDirectBeamAlg->getProperty("OutputWorkspace");

  directBeamWS = convertToWavelength(directBeamWS);

  return directBeamWS;
}

/**
* Normalize the workspace by the direct beam (optional)
*
* @param detectorWS : workspace in wavelength which is to be normalized
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::directBeamCorrection(
    MatrixWorkspace_sptr detectorWS) {

  MatrixWorkspace_sptr normalized = detectorWS;
  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (!directBeamProperty->isDefault()) {
    const auto directBeam = makeDirectBeamWS(m_runWS);
    normalized = divide(detectorWS, directBeam);
  }

  return normalized;
}

/**
* Perform either transmission or algorithmic correction according to the
* settings.
* @param detectorWS : workspace in wavelength which is to be normalized
* @param detectorWSReduced:: whether the input detector workspace has been
* reduced
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::transOrAlgCorrection(
    MatrixWorkspace_sptr detectorWS, const bool detectorWSReduced) {

  MatrixWorkspace_sptr normalized;
  MatrixWorkspace_sptr transRun = getProperty("FirstTransmissionRun");
  if (transRun) {
    normalized = transmissionCorrection(detectorWS, detectorWSReduced);
  } else if (getPropertyValue("CorrectionAlgorithm") != "None") {
    normalized = algorithmicCorrection(detectorWS);
  } else {
    normalized = detectorWS;
  }

  return normalized;
}

/** Perform transmission correction by running 'CreateTransmissionWorkspace' on
* the input workspace
* @param detectorWS :: the input workspace
* @param detectorWSReduced:: whether the input detector workspace has been
* reduced
* @return :: the input workspace normalized by transmission
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::transmissionCorrection(
    MatrixWorkspace_sptr detectorWS, const bool detectorWSReduced) {

  const bool strictSpectrumChecking = getProperty("StrictSpectrumChecking");
  MatrixWorkspace_sptr transmissionWS = getProperty("FirstTransmissionRun");

  // Reduce the transmission workspace, if not already done (assume that if
  // the workspace is in wavelength then it has already been reduced)
  Unit_const_sptr xUnit = transmissionWS->getAxis(0)->unit();
  if (xUnit->unitID() == "TOF") {

    // Processing instructions for transmission workspace. If strict spectrum
    // checking is not enabled then just use the same processing instructions
    // that were passed in.
    std::string transmissionCommands = getProperty("ProcessingInstructions");
    if (strictSpectrumChecking) {
      // If we have strict spectrum checking, we should have the same
      // spectrum numbers in both workspaces, but not necessarily with the
      // same workspace indices. Therefore, map the processing instructions
      // from the original workspace to the correct indices in the
      // transmission workspace. Note that we use the run workspace here
      // because the detectorWS may already have been reduced and may not
      // contain the original spectra.
      transmissionCommands = createProcessingCommandsFromDetectorWS(
          m_runWS, transmissionWS, detectorGroups());
    }

    MatrixWorkspace_sptr secondTransmissionWS =
        getProperty("SecondTransmissionRun");
    auto alg = this->createChildAlgorithm("CreateTransmissionWorkspace");
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", transmissionWS);
    alg->setProperty("SecondTransmissionRun", secondTransmissionWS);
    alg->setPropertyValue("Params", getPropertyValue("Params"));
    alg->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
    alg->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
    alg->setPropertyValue("I0MonitorIndex", getPropertyValue("I0MonitorIndex"));
    alg->setPropertyValue("WavelengthMin", getPropertyValue("WavelengthMin"));
    alg->setPropertyValue("WavelengthMax", getPropertyValue("WavelengthMax"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMin",
                          getPropertyValue("MonitorBackgroundWavelengthMin"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMax",
                          getPropertyValue("MonitorBackgroundWavelengthMax"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMin",
                          getPropertyValue("MonitorIntegrationWavelengthMin"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMax",
                          getPropertyValue("MonitorIntegrationWavelengthMax"));
    alg->setProperty("ProcessingInstructions", transmissionCommands);
    alg->execute();
    transmissionWS = alg->getProperty("OutputWorkspace");
  }

  // Rebin the transmission run to be the same as the input.
  auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspaceAlg->initialize();
  rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
  rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", transmissionWS);
  rebinToWorkspaceAlg->execute();
  transmissionWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

  // If the detector workspace has been reduced then the spectrum maps
  // should match AFTER reducing the transmission workspace
  if (detectorWSReduced) {
    verifySpectrumMaps(detectorWS, transmissionWS, strictSpectrumChecking);
  }

  MatrixWorkspace_sptr normalized = divide(detectorWS, transmissionWS);
  return normalized;
}

/**
* Perform transmission correction using alternative correction algorithms
* @param detectorWS : workspace in wavelength which is to be normalized by the
* results of the transmission corrections.
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::algorithmicCorrection(
    MatrixWorkspace_sptr detectorWS) {

  const std::string corrAlgName = getProperty("CorrectionAlgorithm");

  IAlgorithm_sptr corrAlg = createChildAlgorithm(corrAlgName);
  corrAlg->initialize();
  if (corrAlgName == "PolynomialCorrection") {
    corrAlg->setPropertyValue("Coefficients", getPropertyValue("Polynomial"));
  } else if (corrAlgName == "ExponentialCorrection") {
    corrAlg->setPropertyValue("C0", getPropertyValue("C0"));
    corrAlg->setPropertyValue("C1", getPropertyValue("C1"));
  } else {
    throw std::runtime_error("Unknown correction algorithm: " + corrAlgName);
  }

  corrAlg->setProperty("InputWorkspace", detectorWS);
  corrAlg->setProperty("Operation", "Divide");
  corrAlg->execute();

  return corrAlg->getProperty("OutputWorkspace");
}

/**
* The input workspace (in wavelength) to convert to Q
* @param inputWS : the input workspace to convert
* @return : output workspace in Q
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::convertToQ(MatrixWorkspace_sptr inputWS) {

  // Convert to Q
  auto convertUnits = this->createChildAlgorithm("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", inputWS);
  convertUnits->setProperty("Target", "MomentumTransfer");
  convertUnits->setProperty("AlignBins", false);
  convertUnits->execute();
  MatrixWorkspace_sptr IvsQ = convertUnits->getProperty("OutputWorkspace");

  return IvsQ;
}

/**
* Determine whether the reduction should sum along lines of constant
* Q or in the default lambda.
*
* @return : true if the reduction should sum in Q; false otherwise
*/
bool ReflectometryReductionOne2::summingInQ() {
  bool result = false;
  const std::string summationType = getProperty("SummationType");

  if (summationType == "SumInQ") {
    result = true;
  }

  return result;
}

/**
* Find and cache the indicies of the detectors of interest
*/
void ReflectometryReductionOne2::findDetectorGroups() {
  std::string instructions = getPropertyValue("ProcessingInstructions");

  m_detectorGroups = translateInstructions(instructions);

  if (m_detectorGroups.size() == 0) {
    throw std::runtime_error("Invalid processing instructions");
  }
}

/**
* Find and cache the angle theta0 from which lines of constant Q emanate
*/
void ReflectometryReductionOne2::findTheta0() {
  // Only requried if summing in Q
  if (!summingInQ()) {
    return;
  }

  const std::string reductionType = getProperty("ReductionType");

  // For the non-flat sample case theta0 is 0
  m_theta0 = 0.0;

  if (reductionType == "DivergentBeam") {
    // theta0 is the horizon angle, which is half the twoTheta angle of the
    // detector position. This is the angle the detector has been rotated
    // to, which we can get from ThetaIn
    Property *thetaIn = getProperty("ThetaIn");
    if (!thetaIn->isDefault()) {
      m_theta0 = getProperty("ThetaIn");
    } else {
      /// @todo Find the value of theta using calculateTheta (which could
      /// be moved to the base class from ReflectometryReductionOneAuto)
      throw std::runtime_error(
          "The ThetaIn property is required for the DivergentBeam case");
    }
  }

  g_log.debug() << "theta0: " << theta0() << " degrees" << std::endl;

  // Convert to radians
  m_theta0 *= M_PI / 180.0;
}

/**
* Get the (arbitrary) reference angle twoThetaR for use for summation
* in Q
*
* @return : the angle twoThetaR in radians
* @throws : if the angle could not be found
*/
double
ReflectometryReductionOne2::twoThetaR(const std::vector<size_t> &detectors) {
  return getDetectorTwoTheta(m_spectrumInfo, twoThetaRDetectorIdx(detectors));
}

/**
* Get the spectrum index which defines the twoThetaR reference angle
* @return : the spectrum index
*/
size_t ReflectometryReductionOne2::twoThetaRDetectorIdx(
    const std::vector<size_t> &detectors) {
  // Get the mid-point of the area of interest
  return detectors.front() + (detectors.back() - detectors.front()) / 2;
}

/**
* Construct an "empty" output workspace in virtual-lambda for summation in Q.
* The workspace will have the same x values as the input workspace but the y
* values will all be zero.
*
* @return : a 1D workspace where y values are all zero
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::constructIvsLamWS(
    MatrixWorkspace_sptr detectorWS, const std::vector<size_t> &detectors) {
  // Construct the new workspace from the centre pixel (where twoThetaR is
  // defined)
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
  cropWorkspaceAlg->setProperty(
      "StartWorkspaceIndex", static_cast<int>(twoThetaRDetectorIdx(detectors)));
  cropWorkspaceAlg->setProperty(
      "EndWorkspaceIndex", static_cast<int>(twoThetaRDetectorIdx(detectors)));
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr ws = cropWorkspaceAlg->getProperty("OutputWorkspace");

  // Get the max/min wavelength of region of interest
  const double lambdaMin = getProperty("WavelengthMin");
  const double lambdaMax = getProperty("WavelengthMax");

  // Get the max and min values of the projected (virtual) lambda range
  double lambdaVMin = 0.0;
  double lambdaVMax = 0.0;
  double dummy = 0.0;

  const size_t spIdxMin = detectors.front();
  const double twoThetaMin = getDetectorTwoTheta(m_spectrumInfo, spIdxMin);
  const double bTwoThetaMin =
      getDetectorTwoThetaRange(m_spectrumInfo, spIdxMin);
  // For bLambda, use the average bin size for this spectrum
  auto xValues = detectorWS->x(spIdxMin);
  double bLambda = (xValues[xValues.size() - 1] - xValues[0]) /
                   static_cast<int>(xValues.size());
  getProjectedLambdaRange(lambdaMax, twoThetaMin, bLambda, bTwoThetaMin,
                          detectors, lambdaVMin, dummy);

  const size_t spIdxMax = detectors.back();
  const double twoThetaMax = getDetectorTwoTheta(m_spectrumInfo, spIdxMax);
  const double bTwoThetaMax =
      getDetectorTwoThetaRange(m_spectrumInfo, spIdxMax);
  xValues = detectorWS->x(spIdxMax);
  bLambda = (xValues[xValues.size() - 1] - xValues[0]) /
            static_cast<int>(xValues.size());
  getProjectedLambdaRange(lambdaMin, twoThetaMax, bLambda, bTwoThetaMax,
                          detectors, dummy, lambdaVMax);

  if (lambdaVMin > lambdaVMax) {
    std::swap(lambdaVMin, lambdaVMax);
  }

  // Use the same number of bins as the input
  const int origNumBins = static_cast<int>(detectorWS->blocksize());
  const double binWidth = (lambdaMax - lambdaMin) / origNumBins;
  std::string params(std::to_string(lambdaVMin) + "," +
                     std::to_string(binWidth) + "," +
                     std::to_string(lambdaVMax));

  auto rebinAlg = this->createChildAlgorithm("Rebin");
  rebinAlg->initialize();
  rebinAlg->setProperty("InputWorkspace", ws);
  rebinAlg->setProperty("PreserveEvents", false);
  rebinAlg->setProperty("Params", params);
  rebinAlg->execute();
  ws = rebinAlg->getProperty("OutputWorkspace");

  for (int i = 0; i < static_cast<int>(ws->blocksize()); ++i) {
    ws->dataY(0)[i] = 0.0;
    ws->dataE(0)[i] = 0.0;
  }

  return ws;
}

/**
* Sum counts from the input workspace in lambda along lines of constant Q by
* projecting to "virtual lambda" at a reference angle twoThetaR.
*
* @param detectorWS [in] :: the input workspace in wavelength
* @param detectors [in] :: spectrum indices of the detectors of interest
* @return :: the output workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::sumInQ(MatrixWorkspace_sptr detectorWS,
                                   const std::vector<size_t> &detectors) {
  // Construct the output array in virtual lambda
  MatrixWorkspace_sptr IvsLam = constructIvsLamWS(detectorWS, detectors);
  auto &outputE = IvsLam->dataE(0);

  // Loop through each spectrum in the region of interest
  for (size_t spIdx : detectors) {
    // Get the angle of this detector and its size in twoTheta
    const double twoTheta = getDetectorTwoTheta(m_spectrumInfo, spIdx);
    const double bTwoTheta = getDetectorTwoThetaRange(m_spectrumInfo, spIdx);

    // Check X length is Y length + 1
    const auto &inputX = detectorWS->x(spIdx);
    const auto &inputY = detectorWS->y(spIdx);
    const auto &inputE = detectorWS->e(spIdx);
    if (inputX.size() != inputY.size() + 1) {
      throw std::runtime_error(
          "Expected input workspace to be histogram data (got X len=" +
          std::to_string(inputX.size()) + ", Y len=" +
          std::to_string(inputY.size()) + ")");
    }

    // Create a vector for the projected errors for this spectrum.
    // (Output Y values can simply be accumulated directly into the output
    // workspace, but for error values we need to create a separate error
    // vector for the projected errors from each input spectrum and then
    // do an overall sum in quadrature.)
    std::vector<double> projectedE(outputE.size(), 0.0);

    // Process each value in the spectrum
    const int ySize = static_cast<int>(inputY.size());
    for (int inputIdx = 0; inputIdx < ySize; ++inputIdx) {
      // Do the summation in Q
      sumInQProcessValue(inputIdx, twoTheta, bTwoTheta, inputX, inputY, inputE,
                         detectors, IvsLam, projectedE);
    }

    // Sum errors in quadrature
    const int eSize = static_cast<int>(inputE.size());
    for (int outIdx = 0; outIdx < eSize; ++outIdx) {
      outputE[outIdx] += projectedE[outIdx] * projectedE[outIdx];
    }
  }

  // Take the square root of all the accumulated squared errors. Assumes
  // Gaussian errors
  double (*rs)(double) = std::sqrt;
  std::transform(outputE.begin(), outputE.end(), outputE.begin(), rs);

  return IvsLam;
}

/**
* Share counts from an input value onto the projected output in virtual-lambda
*
* @param inputIdx [in] :: the index into the input arrays
* @param twoTheta [in] :: the value of twotTheta for this spectrum
* @param bTwoTheta [in] :: the size of the pixel in twoTheta
* @param inputX [in] :: the input spectrum X values
* @param inputY [in] :: the input spectrum Y values
* @param inputE [in] :: the input spectrum E values
* @param detectors [in] :: spectrum indices of the detectors of interest
* @param IvsLam [in,out] :: the output workspace
* @param outputE [in,out] :: the projected E values
*/
void ReflectometryReductionOne2::sumInQProcessValue(
    const int inputIdx, const double twoTheta, const double bTwoTheta,
    const HistogramX &inputX, const HistogramY &inputY,
    const HistogramE &inputE, const std::vector<size_t> &detectors,
    MatrixWorkspace_sptr IvsLam, std::vector<double> &outputE) {

  // Check whether there are any counts (if not, nothing to share)
  const double inputCounts = inputY[inputIdx];
  if (inputCounts <= 0.0 || std::isnan(inputCounts) ||
      std::isinf(inputCounts)) {
    return;
  }
  // Get the bin width and the bin centre
  const double bLambda = getLambdaRange(inputX, inputIdx);
  const double lambda = getLambda(inputX, inputIdx);
  // Project these coordinates onto the virtual-lambda output (at twoThetaR)
  double lambdaVMin = 0.0;
  double lambdaVMax = 0.0;
  getProjectedLambdaRange(lambda, twoTheta, bLambda, bTwoTheta, detectors,
                          lambdaVMin, lambdaVMax);
  // Share the input counts into the output array
  sumInQShareCounts(inputCounts, inputE[inputIdx], bLambda, lambdaVMin,
                    lambdaVMax, IvsLam, outputE);
}

/**
* Share the given input counts into the output array bins proportionally
* according to how much the bins overlap the given lambda range.
* outputX.size() must equal outputY.size() + 1
*
* @param inputCounts [in] :: the input counts to share out
* @param inputErr [in] :: the input errors to share out
* @param bLambda [in] :: the bin width in lambda
* @param lambdaMin [in] :: the start of the range to share counts to
* @param lambdaMax [in] :: the end of the range to share counts to
* @param IvsLam [in,out] :: the output workspace
* @param outputE [in,out] :: the projected E values
*/
void ReflectometryReductionOne2::sumInQShareCounts(
    const double inputCounts, const double inputErr, const double bLambda,
    const double lambdaMin, const double lambdaMax, MatrixWorkspace_sptr IvsLam,
    std::vector<double> &outputE) {
  // Check that we have histogram data
  const auto &outputX = IvsLam->dataX(0);
  auto &outputY = IvsLam->dataY(0);
  if (outputX.size() != outputY.size() + 1) {
    throw std::runtime_error(
        "Expected output array to be histogram data (got X len=" +
        std::to_string(outputX.size()) + ", Y len=" +
        std::to_string(outputY.size()) + ")");
  }

  const double totalWidth = lambdaMax - lambdaMin;

  // Get the first bin edge in the output X array that is within range.
  // There will probably be some overlap, so start from the bin edge before
  // this (unless we're already at the first bin edge).
  auto startIter = std::lower_bound(outputX.begin(), outputX.end(), lambdaMin);
  if (startIter != outputX.begin()) {
    --startIter;
  }

  // Loop through all overlapping output bins. Convert the iterator to an
  // index because we need to index both the X and Y arrays.
  const int xSize = static_cast<int>(outputX.size());
  for (auto outIdx = startIter - outputX.begin(); outIdx < xSize - 1;
       ++outIdx) {
    const double binStart = outputX[outIdx];
    const double binEnd = outputX[outIdx + 1];
    if (binStart > lambdaMax) {
      // No longer in the overlap region so we're finished
      break;
    }
    // Add a share of the input counts to this bin based on the proportion of
    // overlap.
    const double overlapWidth =
        std::min({bLambda, lambdaMax - binStart, binEnd - lambdaMin});
    const double fraction = overlapWidth / totalWidth;
    outputY[outIdx] += inputCounts * fraction;
    outputE[outIdx] += inputErr * fraction;
  }
}

/**
* Project an input pixel onto an arbitrary reference line at twoThetaR. The
* projection is done along lines of constant Q, which emanate from theta0. The
* top-left and bottom-right corners of the pixel are projected, resulting in an
* output range in "virtual" lambda (lambdaV).
*
* For a description of this projection, see:
*   R. Cubitt, T. Saerbeck, R.A. Campbell, R. Barker, P. Gutfreund
*   J. Appl. Crystallogr., 48 (6) (2015)
*
* @param lambda [in] :: the lambda coord of the centre of the pixel to project
* @param twoTheta [in] :: the twoTheta coord of the centre of the pixel to
*project
* @param bLambda [in] :: the pixel size in lambda
* @param bTwoTheta [in] :: the pixel size in twoTheta
* @param detectors [in] :: spectrum indices of the detectors of interest
* @param lambdaVMin [out] :: the projected range start
* @param lambdaVMax [out] :: the projected range end
*/
void ReflectometryReductionOne2::getProjectedLambdaRange(
    const double lambda, const double twoTheta, const double bLambda,
    const double bTwoTheta, const std::vector<size_t> &detectors,
    double &lambdaVMin, double &lambdaVMax) {

  // Get the angle from twoThetaR to this detector
  const double twoThetaRVal = twoThetaR(detectors);
  // Get the distance from the pixel to twoThetaR
  const double gamma = twoTheta - twoThetaRVal;
  // Get the angle from the horizon to the reference angle
  const double horizonThetaR = twoThetaRVal - theta0();

  // Calculate the projected wavelength range
  try {
    const double lambdaTop = std::sin(horizonThetaR) *
                             (lambda + bLambda / 2.0) /
                             std::sin(horizonThetaR + gamma - bTwoTheta / 2.0);
    const double lambdaBot = std::sin(horizonThetaR) *
                             (lambda - bLambda / 2.0) /
                             std::sin(horizonThetaR + gamma + bTwoTheta / 2.0);
    lambdaVMin = std::min(lambdaTop, lambdaBot);
    lambdaVMax = std::max(lambdaTop, lambdaBot);
  } catch (std::exception &ex) {
    throw std::runtime_error(
        "Failed to project (lambda, twoTheta) = (" + std::to_string(lambda) +
        "," + std::to_string(twoTheta * 180.0 / M_PI) + ") onto twoThetaR = " +
        std::to_string(twoThetaRVal) + ": " + ex.what());
  }
}

/**
Check whether the spectrum maps for the given workspaces are the same.

@param ws1 : First workspace to compare
@param ws2 : Second workspace to compare against
@param severe: True to indicate that failure to verify should result in an
exception. Otherwise a warning is generated.
*/
void ReflectometryReductionOne2::verifySpectrumMaps(
    MatrixWorkspace_const_sptr ws1, MatrixWorkspace_const_sptr ws2,
    const bool severe) {
  auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
  auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
  if (map1 != map2) {
    const std::string message =
        "Spectrum maps between workspaces do NOT match up.";
    if (severe) {
      throw std::invalid_argument(message);
    } else {
      g_log.warning(message);
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
