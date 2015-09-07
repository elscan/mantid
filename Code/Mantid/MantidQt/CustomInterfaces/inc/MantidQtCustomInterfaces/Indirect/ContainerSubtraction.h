#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_

#include "ui_ContainerSubtraction.h"
#include "CorrectionsTab.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport ContainerSubtraction : public CorrectionsTab {
  Q_OBJECT

public:
  ContainerSubtraction(QWidget *parent = 0);

private slots:
  /// Handles the geometry being changed
  void handleGeometryChange(int index);
  /// Handles a new sample being loaded
  void newData(const QString &dataName);
  /// Updates the preview mini plot
  void plotPreview(int specIndex);
  /// Handle abs. correction algorithm completion
  //void absCorComplete(bool error);
  /// Handle convert units and save algorithm completion
  //void postProcessComplete(bool error);

private:
  virtual void setup();
  virtual void run();
  virtual bool validate();
  virtual void loadSettings(const QSettings &settings);

  Ui::ContainerSubtraction m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONTAINERSUBTRACTION_H_ */