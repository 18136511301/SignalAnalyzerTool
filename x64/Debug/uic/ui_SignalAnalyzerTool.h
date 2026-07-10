/********************************************************************************
** Form generated from reading UI file 'SignalAnalyzerTool.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SIGNALANALYZERTOOL_H
#define UI_SIGNALANALYZERTOOL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SignalAnalyzerToolClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBoxParams;
    QHBoxLayout *horizontalLayoutParams;
    QLabel *labelFile;
    QLineEdit *lineEditFile;
    QPushButton *btnBrowse;
    QLabel *labelSampleRate;
    QDoubleSpinBox *spinSampleRate;
    QLabel *labelSignalType;
    QComboBox *comboSignalType;
    QHBoxLayout *horizontalLayoutButtons;
    QPushButton *btnAnalyze;
    QPushButton *btnDemod;
    QPushButton *btnClear;
    QSplitter *splitterMain;
    QWidget *widgetResults;
    QVBoxLayout *verticalLayoutResults;
    QGroupBox *groupBoxResults;
    QFormLayout *formLayoutResults;
    QLabel *lblCenterFreqTitle;
    QLabel *lblCenterFreq;
    QLabel *lblSymbolRateTitle;
    QLabel *lblSymbolRate;
    QLabel *lblBandwidthTitle;
    QLabel *lblBandwidth;
    QLabel *lblSNRTitle;
    QLabel *lblSNR;
    QLabel *lblModTypeTitle;
    QLabel *lblModType;
    QLabel *lblModOrderTitle;
    QLabel *lblModOrder;
    QGroupBox *groupBoxSymbolRate;
    QFormLayout *formLayoutOverride;
    QLabel *lblOverrideSymbolRate;
    QDoubleSpinBox *spinSymbolRate;
    QLabel *lblOverrideModType;
    QComboBox *comboModType;
    QSpacerItem *verticalSpacer;
    QWidget *widgetPlots;
    QVBoxLayout *verticalLayoutPlots;
    QWidget *plotPSDContainer;
    QWidget *plotConstellationContainer;
    QLabel *lblStatus;
    QTextEdit *textLog;
    QMenuBar *menuBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *SignalAnalyzerToolClass)
    {
        if (SignalAnalyzerToolClass->objectName().isEmpty())
            SignalAnalyzerToolClass->setObjectName(QString::fromUtf8("SignalAnalyzerToolClass"));
        SignalAnalyzerToolClass->resize(1200, 800);
        centralWidget = new QWidget(SignalAnalyzerToolClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        groupBoxParams = new QGroupBox(centralWidget);
        groupBoxParams->setObjectName(QString::fromUtf8("groupBoxParams"));
        horizontalLayoutParams = new QHBoxLayout(groupBoxParams);
        horizontalLayoutParams->setObjectName(QString::fromUtf8("horizontalLayoutParams"));
        labelFile = new QLabel(groupBoxParams);
        labelFile->setObjectName(QString::fromUtf8("labelFile"));

        horizontalLayoutParams->addWidget(labelFile);

        lineEditFile = new QLineEdit(groupBoxParams);
        lineEditFile->setObjectName(QString::fromUtf8("lineEditFile"));
        lineEditFile->setReadOnly(true);

        horizontalLayoutParams->addWidget(lineEditFile);

        btnBrowse = new QPushButton(groupBoxParams);
        btnBrowse->setObjectName(QString::fromUtf8("btnBrowse"));

        horizontalLayoutParams->addWidget(btnBrowse);

        labelSampleRate = new QLabel(groupBoxParams);
        labelSampleRate->setObjectName(QString::fromUtf8("labelSampleRate"));

        horizontalLayoutParams->addWidget(labelSampleRate);

        spinSampleRate = new QDoubleSpinBox(groupBoxParams);
        spinSampleRate->setObjectName(QString::fromUtf8("spinSampleRate"));
        spinSampleRate->setMinimum(1000.000000000000000);
        spinSampleRate->setMaximum(1000000000.000000000000000);
        spinSampleRate->setValue(1000000.000000000000000);

        horizontalLayoutParams->addWidget(spinSampleRate);

        labelSignalType = new QLabel(groupBoxParams);
        labelSignalType->setObjectName(QString::fromUtf8("labelSignalType"));

        horizontalLayoutParams->addWidget(labelSignalType);

        comboSignalType = new QComboBox(groupBoxParams);
        comboSignalType->addItem(QString());
        comboSignalType->addItem(QString());
        comboSignalType->setObjectName(QString::fromUtf8("comboSignalType"));

        horizontalLayoutParams->addWidget(comboSignalType);


        verticalLayout->addWidget(groupBoxParams);

        horizontalLayoutButtons = new QHBoxLayout();
        horizontalLayoutButtons->setObjectName(QString::fromUtf8("horizontalLayoutButtons"));
        btnAnalyze = new QPushButton(centralWidget);
        btnAnalyze->setObjectName(QString::fromUtf8("btnAnalyze"));
        btnAnalyze->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: white; font-weight: bold; padding: 8px;"));

        horizontalLayoutButtons->addWidget(btnAnalyze);

        btnDemod = new QPushButton(centralWidget);
        btnDemod->setObjectName(QString::fromUtf8("btnDemod"));
        btnDemod->setStyleSheet(QString::fromUtf8("background-color: #2196F3; color: white; font-weight: bold; padding: 8px;"));
        btnDemod->setEnabled(false);

        horizontalLayoutButtons->addWidget(btnDemod);

        btnClear = new QPushButton(centralWidget);
        btnClear->setObjectName(QString::fromUtf8("btnClear"));
        btnClear->setStyleSheet(QString::fromUtf8("background-color: #f44336; color: white; padding: 8px;"));

        horizontalLayoutButtons->addWidget(btnClear);


        verticalLayout->addLayout(horizontalLayoutButtons);

        splitterMain = new QSplitter(centralWidget);
        splitterMain->setObjectName(QString::fromUtf8("splitterMain"));
        splitterMain->setOrientation(Qt::Horizontal);
        widgetResults = new QWidget(splitterMain);
        widgetResults->setObjectName(QString::fromUtf8("widgetResults"));
        verticalLayoutResults = new QVBoxLayout(widgetResults);
        verticalLayoutResults->setObjectName(QString::fromUtf8("verticalLayoutResults"));
        verticalLayoutResults->setContentsMargins(0, 0, 0, 0);
        groupBoxResults = new QGroupBox(widgetResults);
        groupBoxResults->setObjectName(QString::fromUtf8("groupBoxResults"));
        formLayoutResults = new QFormLayout(groupBoxResults);
        formLayoutResults->setObjectName(QString::fromUtf8("formLayoutResults"));
        lblCenterFreqTitle = new QLabel(groupBoxResults);
        lblCenterFreqTitle->setObjectName(QString::fromUtf8("lblCenterFreqTitle"));

        formLayoutResults->setWidget(0, QFormLayout::LabelRole, lblCenterFreqTitle);

        lblCenterFreq = new QLabel(groupBoxResults);
        lblCenterFreq->setObjectName(QString::fromUtf8("lblCenterFreq"));

        formLayoutResults->setWidget(0, QFormLayout::FieldRole, lblCenterFreq);

        lblSymbolRateTitle = new QLabel(groupBoxResults);
        lblSymbolRateTitle->setObjectName(QString::fromUtf8("lblSymbolRateTitle"));

        formLayoutResults->setWidget(1, QFormLayout::LabelRole, lblSymbolRateTitle);

        lblSymbolRate = new QLabel(groupBoxResults);
        lblSymbolRate->setObjectName(QString::fromUtf8("lblSymbolRate"));

        formLayoutResults->setWidget(1, QFormLayout::FieldRole, lblSymbolRate);

        lblBandwidthTitle = new QLabel(groupBoxResults);
        lblBandwidthTitle->setObjectName(QString::fromUtf8("lblBandwidthTitle"));

        formLayoutResults->setWidget(2, QFormLayout::LabelRole, lblBandwidthTitle);

        lblBandwidth = new QLabel(groupBoxResults);
        lblBandwidth->setObjectName(QString::fromUtf8("lblBandwidth"));

        formLayoutResults->setWidget(2, QFormLayout::FieldRole, lblBandwidth);

        lblSNRTitle = new QLabel(groupBoxResults);
        lblSNRTitle->setObjectName(QString::fromUtf8("lblSNRTitle"));

        formLayoutResults->setWidget(3, QFormLayout::LabelRole, lblSNRTitle);

        lblSNR = new QLabel(groupBoxResults);
        lblSNR->setObjectName(QString::fromUtf8("lblSNR"));

        formLayoutResults->setWidget(3, QFormLayout::FieldRole, lblSNR);

        lblModTypeTitle = new QLabel(groupBoxResults);
        lblModTypeTitle->setObjectName(QString::fromUtf8("lblModTypeTitle"));

        formLayoutResults->setWidget(4, QFormLayout::LabelRole, lblModTypeTitle);

        lblModType = new QLabel(groupBoxResults);
        lblModType->setObjectName(QString::fromUtf8("lblModType"));

        formLayoutResults->setWidget(4, QFormLayout::FieldRole, lblModType);

        lblModOrderTitle = new QLabel(groupBoxResults);
        lblModOrderTitle->setObjectName(QString::fromUtf8("lblModOrderTitle"));

        formLayoutResults->setWidget(5, QFormLayout::LabelRole, lblModOrderTitle);

        lblModOrder = new QLabel(groupBoxResults);
        lblModOrder->setObjectName(QString::fromUtf8("lblModOrder"));

        formLayoutResults->setWidget(5, QFormLayout::FieldRole, lblModOrder);


        verticalLayoutResults->addWidget(groupBoxResults);

        groupBoxSymbolRate = new QGroupBox(widgetResults);
        groupBoxSymbolRate->setObjectName(QString::fromUtf8("groupBoxSymbolRate"));
        formLayoutOverride = new QFormLayout(groupBoxSymbolRate);
        formLayoutOverride->setObjectName(QString::fromUtf8("formLayoutOverride"));
        lblOverrideSymbolRate = new QLabel(groupBoxSymbolRate);
        lblOverrideSymbolRate->setObjectName(QString::fromUtf8("lblOverrideSymbolRate"));

        formLayoutOverride->setWidget(0, QFormLayout::LabelRole, lblOverrideSymbolRate);

        spinSymbolRate = new QDoubleSpinBox(groupBoxSymbolRate);
        spinSymbolRate->setObjectName(QString::fromUtf8("spinSymbolRate"));
        spinSymbolRate->setMinimum(1.000000000000000);
        spinSymbolRate->setMaximum(100000000.000000000000000);
        spinSymbolRate->setValue(100000.000000000000000);

        formLayoutOverride->setWidget(0, QFormLayout::FieldRole, spinSymbolRate);

        lblOverrideModType = new QLabel(groupBoxSymbolRate);
        lblOverrideModType->setObjectName(QString::fromUtf8("lblOverrideModType"));

        formLayoutOverride->setWidget(1, QFormLayout::LabelRole, lblOverrideModType);

        comboModType = new QComboBox(groupBoxSymbolRate);
        comboModType->addItem(QString());
        comboModType->addItem(QString());
        comboModType->addItem(QString());
        comboModType->addItem(QString());
        comboModType->addItem(QString());
        comboModType->addItem(QString());
        comboModType->setObjectName(QString::fromUtf8("comboModType"));

        formLayoutOverride->setWidget(1, QFormLayout::FieldRole, comboModType);


        verticalLayoutResults->addWidget(groupBoxSymbolRate);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayoutResults->addItem(verticalSpacer);

        splitterMain->addWidget(widgetResults);
        widgetPlots = new QWidget(splitterMain);
        widgetPlots->setObjectName(QString::fromUtf8("widgetPlots"));
        verticalLayoutPlots = new QVBoxLayout(widgetPlots);
        verticalLayoutPlots->setObjectName(QString::fromUtf8("verticalLayoutPlots"));
        verticalLayoutPlots->setContentsMargins(0, 0, 0, 0);
        plotPSDContainer = new QWidget(widgetPlots);
        plotPSDContainer->setObjectName(QString::fromUtf8("plotPSDContainer"));
        plotPSDContainer->setMinimumSize(QSize(0, 250));

        verticalLayoutPlots->addWidget(plotPSDContainer);

        plotConstellationContainer = new QWidget(widgetPlots);
        plotConstellationContainer->setObjectName(QString::fromUtf8("plotConstellationContainer"));
        plotConstellationContainer->setMinimumSize(QSize(0, 250));

        verticalLayoutPlots->addWidget(plotConstellationContainer);

        splitterMain->addWidget(widgetPlots);

        verticalLayout->addWidget(splitterMain);

        lblStatus = new QLabel(centralWidget);
        lblStatus->setObjectName(QString::fromUtf8("lblStatus"));
        lblStatus->setStyleSheet(QString::fromUtf8("color: gray; padding: 4px;"));

        verticalLayout->addWidget(lblStatus);

        textLog = new QTextEdit(centralWidget);
        textLog->setObjectName(QString::fromUtf8("textLog"));
        textLog->setReadOnly(true);
        textLog->setMaximumSize(QSize(16777215, 150));
        textLog->setStyleSheet(QString::fromUtf8("font-family: Consolas; font-size: 10pt; background-color: #1e1e1e; color: #d4d4d4;"));

        verticalLayout->addWidget(textLog);

        SignalAnalyzerToolClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(SignalAnalyzerToolClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1200, 21));
        SignalAnalyzerToolClass->setMenuBar(menuBar);
        statusBar = new QStatusBar(SignalAnalyzerToolClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        SignalAnalyzerToolClass->setStatusBar(statusBar);

        retranslateUi(SignalAnalyzerToolClass);

        QMetaObject::connectSlotsByName(SignalAnalyzerToolClass);
    } // setupUi

    void retranslateUi(QMainWindow *SignalAnalyzerToolClass)
    {
        SignalAnalyzerToolClass->setWindowTitle(QCoreApplication::translate("SignalAnalyzerToolClass", "Signal Analyzer Tool", nullptr));
        groupBoxParams->setTitle(QCoreApplication::translate("SignalAnalyzerToolClass", "Signal Parameters", nullptr));
        labelFile->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Data File:", nullptr));
        lineEditFile->setPlaceholderText(QCoreApplication::translate("SignalAnalyzerToolClass", "Select a .dat file...", nullptr));
        btnBrowse->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Browse", nullptr));
        labelSampleRate->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Sample Rate (Hz):", nullptr));
        labelSignalType->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Signal Type:", nullptr));
        comboSignalType->setItemText(0, QCoreApplication::translate("SignalAnalyzerToolClass", "Complex IQ", nullptr));
        comboSignalType->setItemText(1, QCoreApplication::translate("SignalAnalyzerToolClass", "Real (I only)", nullptr));

        btnAnalyze->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Analyze Signal", nullptr));
        btnDemod->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Demodulate", nullptr));
        btnClear->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Clear", nullptr));
        groupBoxResults->setTitle(QCoreApplication::translate("SignalAnalyzerToolClass", "Analysis Results", nullptr));
        lblCenterFreqTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Center Frequency:", nullptr));
        lblCenterFreq->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        lblSymbolRateTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Symbol Rate:", nullptr));
        lblSymbolRate->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        lblBandwidthTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Bandwidth:", nullptr));
        lblBandwidth->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        lblSNRTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "SNR:", nullptr));
        lblSNR->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        lblModTypeTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Modulation Type:", nullptr));
        lblModType->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        lblModOrderTitle->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Modulation Order:", nullptr));
        lblModOrder->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "-", nullptr));
        groupBoxSymbolRate->setTitle(QCoreApplication::translate("SignalAnalyzerToolClass", "Manual Override", nullptr));
        lblOverrideSymbolRate->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Symbol Rate (sym/s):", nullptr));
        lblOverrideModType->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Modulation Type:", nullptr));
        comboModType->setItemText(0, QCoreApplication::translate("SignalAnalyzerToolClass", "Auto", nullptr));
        comboModType->setItemText(1, QCoreApplication::translate("SignalAnalyzerToolClass", "BPSK", nullptr));
        comboModType->setItemText(2, QCoreApplication::translate("SignalAnalyzerToolClass", "QPSK", nullptr));
        comboModType->setItemText(3, QCoreApplication::translate("SignalAnalyzerToolClass", "8PSK", nullptr));
        comboModType->setItemText(4, QCoreApplication::translate("SignalAnalyzerToolClass", "16QAM", nullptr));
        comboModType->setItemText(5, QCoreApplication::translate("SignalAnalyzerToolClass", "64QAM", nullptr));

        lblStatus->setText(QCoreApplication::translate("SignalAnalyzerToolClass", "Ready", nullptr));
        textLog->setPlaceholderText(QCoreApplication::translate("SignalAnalyzerToolClass", "Debug output will appear here...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SignalAnalyzerToolClass: public Ui_SignalAnalyzerToolClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SIGNALANALYZERTOOL_H
