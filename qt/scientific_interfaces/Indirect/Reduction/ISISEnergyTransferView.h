// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "IndirectDataReduction.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "ui_ISISEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IETViewSubscriber {
public:
  virtual void notifyNewMessage(const QString &message) = 0;
  virtual void notifySaveClicked() = 0;
  virtual void notifyRunClicked() = 0;
  virtual void notifyPlotRawClicked() = 0;
  virtual void notifySaveCustomGroupingClicked() = 0;
  virtual void notifyRunFinished() = 0;
};

class MANTIDQT_INDIRECT_DLL IETView : public QWidget {
  Q_OBJECT

public:
  IETView(IETViewSubscriber *subscriber, QWidget *parent = nullptr);
  ~IETView();

  IETRunData getRunData();
  IETPlotData getPlotData();
  IETSaveData getSaveData();

  std::string getCustomGrouping();

  std::string getGroupOutputOption();
  IndirectPlotOptionsView *getPlotOptionsView();

  std::string getFirstFilename();

  bool isRunFilesValid();
  void validateCalibrationFileType(UserInputValidator &uiv);
  void validateRebinString(UserInputValidator &uiv);

  bool showRebinWidthPrompt();
  void showSaveCustomGroupingDialog(std::string const &customGroupingOutput, std::string const &defaultGroupingFilename,
                                    std::string const &saveDirectory);
  void displayWarning(std::string const &message);

  void setDetailedBalance(double detailedBalance);
  void setRunFilesEnabled(bool enable);
  void setSingleRebin(bool enable);
  void setMultipleRebin(bool enable);
  void setSaveEnabled(bool enable);
  void setPlotTimeIsPlotting(bool plotting);
  void setFileExtensionsByName(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes);
  void setOutputWorkspaces(std::vector<std::string> const &outputWorkspaces);

public slots:
  void setInstrumentDefault(InstrumentData const &instrumentDetails);
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");

private slots:
  void showMessageBox(const QString &message);
  void saveClicked();
  void runClicked();
  void plotRawClicked();
  void saveCustomGroupingClicked();
  void pbRunFinished();

  void handleDataReady();

  void mappingOptionSelected(const QString &groupType);

  void pbRunEditing();
  void pbRunFinding();

private:
  int getGroupingOptionIndex(QString const &option);
  bool isOptionHidden(QString const &option);
  void includeExtraGroupingOption(bool includeOption, QString const &option);

  void setRunEnabled(bool enable);
  void setPlotTimeEnabled(bool enable);
  void setButtonsEnabled(bool enable);

  std::vector<std::string> m_outputWorkspaces;
  Ui::ISISEnergyTransfer m_uiForm;
  IETViewSubscriber *m_subscriber;
};
} // namespace CustomInterfaces
} // namespace MantidQt