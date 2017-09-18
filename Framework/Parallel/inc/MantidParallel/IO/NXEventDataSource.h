#ifndef MANTID_PARALLEL_NXEVENTDATASOURCE_H_
#define MANTID_PARALLEL_NXEVENTDATASOURCE_H_

#include <vector>

#include "MantidParallel/DllConfig.h"

namespace Mantid {
namespace Parallel {
namespace IO {

/** NXEventDataSource : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class IndexType, class TimeZeroType, class TimeOffsetType>
class NXEventDataSource {
public:
  virtual ~NXEventDataSource() = default;

  // Construct NXEVentDataLoader with list of bank names
  virtual void setBankIndex(const size_t bank) = 0;

  virtual const std::vector<IndexType> &eventIndex() const = 0;
  virtual const std::vector<TimeZeroType> &eventTimeZero() const = 0;
  virtual void readEventID(int32_t *event_id, size_t start,
                           size_t count) const = 0;
  virtual void readEventTimeOffset(TimeOffsetType *event_time_offset,
                                   size_t start, size_t count) const = 0;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_NXEVENTDATASOURCE_H_ */
