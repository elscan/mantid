// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/BankPulseTimes.h"

#include <nexus/NeXusFile.hpp>

using namespace Mantid::Kernel;
//===============================================================================================
// BankPulseTimes
//===============================================================================================

/// The first period
const unsigned int BankPulseTimes::FirstPeriod = 1;

//----------------------------------------------------------------------------------------------
/** Constructor. Loads the pulse times from the bank entry of the file
 *
 * @param file :: nexus file open in the right bank entry
 * @param pNumbers :: Period numbers to index into. Index via frame/pulse
 */
BankPulseTimes::BankPulseTimes(::NeXus::File &file, const std::vector<int> &pNumbers) : periodNumbers(pNumbers) {
  file.openData("event_time_zero");
  // Read the offset (time zero)
  // If the offset is not present, use Unix epoch
  if (!file.hasAttr("offset"))
    startTime = "1970-01-01T00:00:00Z";
  else
    file.getAttr("offset", startTime);
  Mantid::Types::Core::DateAndTime start(startTime);
  // Load the seconds offsets

  const auto heldTimeZeroType = file.getInfo().type;
  // Nexus only requires event_time_zero to be a NXNumber, we support two
  // possilites
  if (heldTimeZeroType == ::NeXus::FLOAT64) {
    std::vector<double> seconds;
    file.getData(seconds);
    file.closeData();
    // Now create the pulseTimes
    if (seconds.size() == 0)
      throw std::runtime_error("event_time_zero field has no data!");

    std::transform(seconds.cbegin(), seconds.cend(), std::back_inserter(pulseTimes),
                   [start](double seconds) { return start + seconds; });
  } else if (heldTimeZeroType == ::NeXus::UINT64) {
    std::vector<uint64_t> nanoseconds;
    file.getData(nanoseconds);
    file.closeData();
    // Now create the pulseTimes
    if (nanoseconds.size() == 0)
      throw std::runtime_error("event_time_zero field has no data!");

    std::transform(nanoseconds.cbegin(), nanoseconds.cend(), std::back_inserter(pulseTimes),
                   [start](int64_t nanoseconds) { return start + nanoseconds; });
  } else {
    throw std::invalid_argument("Unsupported type for event_time_zero");
  }
  // Ensure that we always have a consistency between nPulses and
  // periodNumbers containers
  if (pulseTimes.size() != pNumbers.size()) {
    periodNumbers = std::vector<int>(pulseTimes.size(), FirstPeriod);
    ;
  }
}

//----------------------------------------------------------------------------------------------
/** Constructor. Build from a vector of date and times.
 *  Handles a zero-sized vector
 *  @param times
 */
BankPulseTimes::BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times) {
  if (times.size() == 0)
    return;

  pulseTimes = times;
  periodNumbers = std::vector<int>(pulseTimes.size(), FirstPeriod); // TODO we are fixing this at 1 period for all
}

//----------------------------------------------------------------------------------------------
/** Comparison. Is this bank's pulse times array the same as another one.
 *
 * @param otherNumPulse :: number of pulses in the OTHER bank event_time_zero.
 * @param otherStartTime :: "offset" attribute of the OTHER bank
 * event_time_zero.
 * @return true if the pulse times are the same and so don't need to be
 * reloaded.
 */
bool BankPulseTimes::equals(size_t otherNumPulse, const std::string &otherStartTime) {
  return ((this->startTime == otherStartTime) && (this->pulseTimes.size() == otherNumPulse));
}
