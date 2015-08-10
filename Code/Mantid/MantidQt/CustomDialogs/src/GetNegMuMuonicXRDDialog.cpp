#include "MantidQtCustomDialogs/GetNegMuMuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomDialogs/MantidGLWidget.h"
#include <QMessageBox>
#include <QLineEdit>
#include <QValidator>
#include <QFormLayout>

namespace MantidQt {
namespace CustomDialogs {
DECLARE_DIALOG(GetNegMuMuonicXRDDialog)

GetNegMuMuonicXRDDialog::GetNegMuMuonicXRDDialog(QWidget *parent)
    : API::AlgorithmDialog(parent) {}

void GetNegMuMuonicXRDDialog::initLayout() {
  auto *periodicTable = new PeriodicTableWidget();
  auto *yPosition = new QLineEdit();
  periodicTable->disableAllElementButtons();

  /*Elements Enabled Correspond to those for which we
  * have data for in the dictionary found in 
  * GetNegMuMuonicXRD.py file
  */
  enableElementsForGetNegMuMuonicXRD(periodicTable);
  auto *main_layout = new QVBoxLayout(this);
  auto *runButton = new QPushButton("Run");
  auto *yPositionLabel = new QLabel("Y Position");
  auto yPositionNumericValidator = new QDoubleValidator();
  /*YPosition LineEdit Attributes*/
  yPosition->setMaximumWidth(250);
  yPosition->setPlaceholderText("-0.01");
  yPosition->setValidator(yPositionNumericValidator);
  /*Run Button*/
  runButton->setMaximumWidth(100);
  connect(runButton, SIGNAL(clicked()), this, SLOT(runClicked(periodicTable, yPosition)));
  connect(this, SIGNAL(validInput()), this, SLOT(accept()));

  /*Adding to Layout*/
  main_layout->addWidget(periodicTable);
  main_layout->addWidget(yPositionLabel);
  main_layout->addWidget(yPosition);
  main_layout->addWidget(runButton);
}

void GetNegMuMuonicXRDDialog::enableElementsForGetNegMuMuonicXRD(PeriodicTableWidget *periodicTable){
    periodicTable->enableButtonByName("Au");
    periodicTable->enableButtonByName("Ag");
    periodicTable->enableButtonByName("Cu");
    periodicTable->enableButtonByName("Zn");
    periodicTable->enableButtonByName("Pb");
    periodicTable->enableButtonByName("As");
    periodicTable->enableButtonByName("Sn");
}

bool GetNegMuMuonicXRDDialog::validateDialogInput(QString input) {
  return (input != "");
}

void GetNegMuMuonicXRDDialog::runClicked(PeriodicTableWidget *p, QLineEdit *yPosition) {
  if (yPosition->text() == "") {
    QMessageBox::information(this, "GetNegMuMuonicXRDDialog",
                             "No Y Axis Position was specified, please enter a "
                             "value or run with default value of -0.001");
  }
  QString elementsSelectedStr = p->getAllCheckedElementsStr();
  if (elementsSelectedStr == "") {
    QMessageBox::information(
        this, "GetNegMuMuonicXRDDialog",
        "No elements were selected, Please select an element from the table");
  }
  if (validateDialogInput(elementsSelectedStr) &&
      validateDialogInput(yPosition->text())) {
    storePropertyValue("Elements", elementsSelectedStr);
    storePropertyValue("YAxisPosition", yPosition->text());
    emit validInput();
  }
  yPosition->setText("-0.001");
}
}
}
