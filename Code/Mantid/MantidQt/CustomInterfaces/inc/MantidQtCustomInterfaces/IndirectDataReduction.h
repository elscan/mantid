#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDataReduction.h"

#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //-------------------------------------------
    // Forward declarations
    //-------------------------------------------
    class IndirectDataReductionTab;

    /** 
    This class defines the IndirectDataReduction interface. It handles the overall instrument settings
    and sets up the appropriate interface depending on the deltaE mode of the instrument. The deltaE
    mode is defined in the instrument definition file using the "deltaE-mode".    

    @author Martyn Gigg, Tessella Support Services plc
    @author Michael Whitty

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    class IndirectDataReduction : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// Default Constructor
      IndirectDataReduction(QWidget *parent = 0);
      ///Destructor
      ~IndirectDataReduction();
      /// Interface name
      static std::string name() { return "Data Reduction"; }
      // This interface's categories.
      static QString categoryInfo() { return "Indirect"; }

      /// Initialize the layout
      virtual void initLayout();
      /// run Python-based initialisation commands
      virtual void initLocalPython();
      /// gather necessary information from Instument Definition Files
      virtual void setIDFValues(const QString & prefix);
      /// perform any instrument-specific changes to layout
      void performInstSpecific();

      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf); ///< handle POCO event

    private slots:
      void helpClicked();
      void runClicked();
      void instrumentLoadingDone(bool error);

      void userSelectInstrument(const QString& name);
      void openDirectoryDialog();
      void showMessageBox(const QString& message); /// Slot showing a message box to the user

    private:
      void loadSettings();
      void readSettings();
      void saveSettings();
      void setDefaultInstrument(const QString & name);
      void instrumentSelectChanged(const QString& name);
      ///// Find path to instrument's _Definition.xml file (and check there is a parameter file).
      //QString getIDFPath(const QString& prefix);
      /// set and show an instrument-specific widget
      void setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState);
      virtual void closeEvent(QCloseEvent* close);

      /// The .ui form generated by Qt Designer
      Ui::IndirectDataReduction m_uiForm;
      /// Instrument the interface is currently set for.
      QString m_curInterfaceSetup;
      /// The settings group
      QString m_settingsGroup;
      /// Runner for insturment load algorithm
      MantidQt::API::AlgorithmRunner* m_algRunner;

      //All indirect tabs
      IndirectDataReductionTab* m_tab_convert_to_energy;
      IndirectDataReductionTab* m_tab_sqw;
      IndirectDataReductionTab* m_tab_diagnostics;
      IndirectDataReductionTab* m_tab_calibration;
      IndirectDataReductionTab* m_tab_trans;
      IndirectDataReductionTab* m_tab_moments;
      IndirectDataReductionTab* m_tab_symmetrise;

      Poco::NObserver<IndirectDataReduction, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver; ///< Poco observer for changes in user directory settings
      QString m_dataDir; ///< default data search directory
      QString m_saveDir; ///< default data save directory
    };

  }
}

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
