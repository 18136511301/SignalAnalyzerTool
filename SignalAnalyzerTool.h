#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QVector>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include "ui_SignalAnalyzerTool.h"
#include "qcustomplot.h"

#include "liquid/liquid_dll.h"
using namespace liquid;

class SignalAnalyzerTool : public QMainWindow
{
    Q_OBJECT

public:
    SignalAnalyzerTool(QWidget *parent = nullptr);
    ~SignalAnalyzerTool();

private slots:
    void onBrowseFile();
    void onAnalyze();
    void onDemodulate();
    void onClear();

private:
    void loadFile(const QString& filePath);
    void plotPSD();
    void plotWaterfall();
    void plotConstellation(const QVector<ComplexFloat>& points);
    void updateResults(const SignalInfo& info);
    void setStatus(const QString& msg);
    void appendLog(const QString& msg);
    QString modTypeName(int type);
    int modTypeFromName(const QString& name);

    void createDemodStatusWidgets();
    void updateDemodStatus(bool locked, unsigned int symbols, float freqOffset, 
                           float symbolRate, qint64 elapsedMs);
    void ensureSquareConstellation();

    Ui::SignalAnalyzerToolClass ui;

    // Tabs
    QTabWidget* m_tabWidget;

    // QCustomPlot widgets
    QCustomPlot* m_plotPSD;
    QCustomPlot* m_plotWaterfall;
    QCustomPlot* m_plotConstellation;

    // Demodulation status labels
    QLabel* m_lblDemodLock;
    QLabel* m_lblDemodSymbols;
    QLabel* m_lblDemodFreqOffset;
    QLabel* m_lblDemodSymbolRate;
    QLabel* m_lblDemodTime;
    QGroupBox* m_groupBoxDemod;

    // Data
    QVector<float> m_iqData;
    bool m_isComplex;
    float m_sampleRate;
    QString m_filePath;
    SignalInfo m_lastInfo;
    bool m_hasResults;

    // Waterfall data
    QVector<QVector<double>> m_waterfallData;
};
