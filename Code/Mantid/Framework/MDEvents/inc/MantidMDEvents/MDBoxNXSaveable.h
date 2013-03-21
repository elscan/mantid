#ifndef MANTID_MDEVENTS_MDBOX_NEXUSSAVEABLE_H
#define MANTID_MDEVENTS_MDBOX_SAVEABLE_H

#include "MantidKernel/Saveable.h"
#include "MantidAPI/IMDNode.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** The class responsible for implementing methods which automatically save/load MDBox into NEXus in conjuction with 
      DiskBuffer

      @date March 15, 2013

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport MDBoxNXSaveable : public Kernel::Saveable
    {
        public:
            MDBoxNXSaveable(API::IMDNode *const);

            /// Save the data to the place, specified by the object
            virtual void save();

            /// Load the data which are not in memory yet and merge them with the data in memory;
            virtual void load();
            /// Method to flush the data to disk and ensure it is written.
            virtual void flushData() const = 0;
            /// remove objects data from memory !!!! wrond overload 
            virtual void clearDataFromMemory()
            {m_MDNode->clear();}

   
           /// @return the amount of memory that the object takes up in the MRU.
           virtual uint64_t getTotalDataSize() const
                 { return m_MDNode->getTotalDataSize(); }
           /** @return the size of the event vector. ! Note that this is NOT necessarily the same as the number of points 
                   (because it might be cached to disk) or the size on disk (because you might have called AddEvents) */
           virtual size_t getDataMemorySize()const 
           {  return m_MDNode->getDataInMemorySize();}
        private:
            API::IMDNode *m_MDNode;
    };

   static void prepareEventNexusData(::NeXus::File * file,const size_t DataChunk,const size_t nColumns,const std::string &descr);

  //---------------------------------------------------------------------------------------------
    /** Open the NXS event data blocks for loading.
     *
     * @param file :: open NXS file.
     * @return the number of events currently in the data field.
     */
  static uint64_t openEventNexusData(::NeXus::File * file);

  static void closeNexusData(::NeXus::File * file);
  void initEventFileStorage(::NeXus::File *hFile,API::BoxController_sptr bc,bool MakeFileBacked,const std::string &EventType);
  void initEventFileStorage(const std::string &fileName,API::BoxController_sptr bc,bool FileBacked,const std::string &EventType);



}
}

#endif