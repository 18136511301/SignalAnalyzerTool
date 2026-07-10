#include "SignalAnalyzerTool.h"
#include <cmath>
#include <complex>
#include <QDir>
#include <QFileInfo>
#include <QSplitter>
#include <QHBoxLayout>

SignalAnalyzerTool::SignalAnalyzerTool(QWidget *parent)
    : QMainWindow(parent), m_isComplex(true), m_sampleRate(100000.0f), m_hasResults(false)
{
    ui.setupUi(this);
    setWindowTitle("Signal Analyzer Tool v2.0");

    // ====== Global style ======
    setStyleSheet(
        "QMainWindow { background-color: #2b2b2b; }"
        "QTabWidget::pane { background: #2b2b2b; border: 1px solid #555; }"
        "QTabBar::tab { background: #3c3c3c; color: #aaa; padding: 8px 20px;"
        "  border: 1px solid #555; border-bottom: none; border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #2b2b2b; color: #4fc3f7; font-weight: bold; }"
        "QLabel { color: #d4d4d4; }"
        "QGroupBox { color: #4fc3f7; font-weight: bold; border: 1px solid #555;"
        "  border-radius: 4px; margin-top: 10px; padding-top: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QDoubleSpinBox { background: #3c3c3c; color: #d4d4d4; border: 1px solid #555;"
        "  border-radius: 3px; padding: 2px 4px; }"
        "QComboBox { background: #3c3c3c; color: #d4d4d4; border: 1px solid #555;"
        "  border-radius: 3px; padding: 2px 8px; }"
        "QLineEdit { background: #3c3c3c; color: #d4d4d4; border: 1px solid #555;"
        "  border-radius: 3px; padding: 2px 4px; }"
    );

    // ====== Button styles ======
    ui.btnAnalyze->setStyleSheet(
        "QPushButton { background-color: #388e3c; color: white; font-weight: bold;"
        "  padding: 10px 20px; border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background-color: #43a047; }"
        "QPushButton:pressed { background-color: #2e7d32; }"
    );
    ui.btnDemod->setStyleSheet(
        "QPushButton { background-color: #1976d2; color: white; font-weight: bold;"
        "  padding: 10px 20px; border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background-color: #1e88e5; }"
        "QPushButton:pressed { background-color: #1565c0; }"
        "QPushButton:disabled { background-color: #555; color: #888; }"
    );
    ui.btnClear->setStyleSheet(
        "QPushButton { background-color: #c62828; color: white; padding: 10px 20px;"
        "  border-radius: 4px; font-size: 13px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
        "QPushButton:pressed { background-color: #b71c1c; }"
    );

    // Connect signals
    connect(ui.btnBrowse, &QPushButton::clicked, this, &SignalAnalyzerTool::onBrowseFile);
    connect(ui.btnAnalyze, &QPushButton::clicked, this, &SignalAnalyzerTool::onAnalyze);
    connect(ui.btnDemod, &QPushButton::clicked, this, &SignalAnalyzerTool::onDemodulate);
    connect(ui.btnClear, &QPushButton::clicked, this, &SignalAnalyzerTool::onClear);

    // Remove old splitter from layout, we'll use tabs instead
    QVBoxLayout* mainLayout = ui.verticalLayout;  // the central widget's layout
    mainLayout->removeWidget(ui.splitterMain);
    ui.splitterMain->hide();  // keep alive, children reparented into tabs

    // ====== Create tab widget ======
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabPosition(QTabWidget::North);

    // ---- Tab 1: 信号识别 ----
    QWidget* tabIdentify = new QWidget();
    QHBoxLayout* idLayout = new QHBoxLayout(tabIdentify);
    idLayout->setContentsMargins(4, 4, 4, 4);

    // Left: Analysis Results
    QWidget* idLeft = new QWidget();
    QVBoxLayout* idLeftLayout = new QVBoxLayout(idLeft);
    idLeftLayout->setContentsMargins(0, 0, 0, 0);
    // Move results group from ui
    ui.verticalLayoutResults->removeWidget(ui.groupBoxResults);
    idLeftLayout->addWidget(ui.groupBoxResults);
    idLeftLayout->addStretch();
    idLeft->setFixedWidth(280);

    // Right: PSD + Waterfall
    QWidget* idRight = new QWidget();
    QVBoxLayout* idRightLayout = new QVBoxLayout(idRight);
    idRightLayout->setContentsMargins(0, 0, 0, 0);

    // PSD plot
    m_plotPSD = new QCustomPlot();
    m_plotPSD->addGraph();
    m_plotPSD->xAxis->setLabel("Frequency (Hz)");
    m_plotPSD->yAxis->setLabel("Power (dB)");
    m_plotPSD->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_plotPSD->plotLayout()->insertRow(0);
    m_plotPSD->plotLayout()->addElement(0, 0,
        new QCPTextElement(m_plotPSD, "Power Spectral Density",
                           QFont("Segoe UI", 10, QFont::Bold)));
    m_plotPSD->setBackground(QColor(30, 30, 30));
    m_plotPSD->xAxis->setLabelColor(QColor(200, 200, 200));
    m_plotPSD->yAxis->setLabelColor(QColor(200, 200, 200));
    m_plotPSD->xAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotPSD->yAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotPSD->xAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotPSD->yAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotPSD->xAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotPSD->yAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotPSD->graph(0)->setPen(QPen(QColor(76, 175, 80), 1.5));
    m_plotPSD->setMinimumHeight(200);

    // Waterfall/Spectrogram
    m_plotWaterfall = new QCustomPlot();
    m_plotWaterfall->plotLayout()->insertRow(0);
    m_plotWaterfall->plotLayout()->addElement(0, 0,
        new QCPTextElement(m_plotWaterfall, "Waterfall / Spectrogram",
                           QFont("Segoe UI", 10, QFont::Bold)));
    m_plotWaterfall->setBackground(QColor(30, 30, 30));
    m_plotWaterfall->xAxis->setLabel("Frequency (Hz)");
    m_plotWaterfall->yAxis->setLabel("Time");
    m_plotWaterfall->xAxis->setLabelColor(QColor(200, 200, 200));
    m_plotWaterfall->yAxis->setLabelColor(QColor(200, 200, 200));
    m_plotWaterfall->xAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotWaterfall->yAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotWaterfall->xAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotWaterfall->yAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotWaterfall->xAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotWaterfall->yAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotWaterfall->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_plotWaterfall->setMinimumHeight(200);
    m_plotWaterfall->yAxis->setLabel("Time (s)");
    m_plotWaterfall->yAxis->setRange(0, 1.0);

    // Create color map for waterfall
    m_plotWaterfall->addGraph();

    // Link PSD x-axis to waterfall x-axis, with range clamp [0, fs/2]
    connect(m_plotPSD->xAxis, qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            this, [this](const QCPRange& range) {
        double lo = qMax(0.0, range.lower);
        double hi = qMin(m_sampleRate / 2.0, range.upper);
        if (lo != range.lower || hi != range.upper)
            m_plotPSD->xAxis->setRange(lo, hi);
        else {
            m_plotWaterfall->xAxis->setRange(lo, hi);
            m_plotWaterfall->replot();
        }
    });

    idRightLayout->addWidget(m_plotPSD, 1);
    idRightLayout->addWidget(m_plotWaterfall, 1);

    idLayout->addWidget(idLeft);
    idLayout->addWidget(idRight, 1);

    // ---- Tab 2: 信号解调 ----
    QWidget* tabDemod = new QWidget();
    QHBoxLayout* demodTabLayout = new QHBoxLayout(tabDemod);
    demodTabLayout->setContentsMargins(4, 4, 4, 4);

    // Left: Manual Override + Demod Status
    QWidget* demodLeft = new QWidget();
    QVBoxLayout* demodLeftLayout = new QVBoxLayout(demodLeft);
    demodLeftLayout->setContentsMargins(0, 0, 0, 0);
    // Move from ui
    ui.verticalLayoutResults->removeWidget(ui.groupBoxSymbolRate);
    demodLeftLayout->addWidget(ui.groupBoxSymbolRate);
    // Create and add demod status
    createDemodStatusWidgets();
    // Move the demod group from resultsLayout to here
    ui.verticalLayoutResults->removeWidget(m_groupBoxDemod);
    demodLeftLayout->addWidget(m_groupBoxDemod);
    demodLeftLayout->addStretch();
    demodLeft->setFixedWidth(280);

    // Right: Constellation
    QWidget* demodRight = new QWidget();
    QVBoxLayout* demodRightLayout = new QVBoxLayout(demodRight);
    demodRightLayout->setContentsMargins(0, 0, 0, 0);

    m_plotConstellation = new QCustomPlot();
    m_plotConstellation->addGraph();
    m_plotConstellation->graph(0)->setLineStyle(QCPGraph::lsNone);
    m_plotConstellation->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(76, 175, 80), QColor(76, 175, 80), 3));
    m_plotConstellation->xAxis->setLabel("In-Phase (I)");
    m_plotConstellation->yAxis->setLabel("Quadrature (Q)");
    m_plotConstellation->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_plotConstellation->xAxis->setRange(-2, 2);
    m_plotConstellation->yAxis->setRange(-2, 2);
    m_plotConstellation->plotLayout()->insertRow(0);
    m_plotConstellation->plotLayout()->addElement(0, 0,
        new QCPTextElement(m_plotConstellation, "Constellation Diagram",
                           QFont("Segoe UI", 10, QFont::Bold)));
    m_plotConstellation->setBackground(QColor(30, 30, 30));
    m_plotConstellation->xAxis->setLabelColor(QColor(200, 200, 200));
    m_plotConstellation->yAxis->setLabelColor(QColor(200, 200, 200));
    m_plotConstellation->xAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotConstellation->yAxis->setTickLabelColor(QColor(180, 180, 180));
    m_plotConstellation->xAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotConstellation->yAxis->setBasePen(QPen(QColor(100, 100, 100)));
    m_plotConstellation->xAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotConstellation->yAxis->setTickPen(QPen(QColor(100, 100, 100)));
    m_plotConstellation->xAxis->grid()->setPen(QPen(QColor(60, 60, 60), 0.5));
    m_plotConstellation->yAxis->grid()->setPen(QPen(QColor(60, 60, 60), 0.5));
    // Connect for square aspect ratio
    connect(m_plotConstellation->xAxis, qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            this, [this](const QCPRange&) { ensureSquareConstellation(); });
    connect(m_plotConstellation->yAxis, qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            this, [this](const QCPRange&) { ensureSquareConstellation(); });

    demodRightLayout->addWidget(m_plotConstellation, 1);

    demodTabLayout->addWidget(demodLeft);
    demodTabLayout->addWidget(demodRight, 1);

    // Add tabs
    m_tabWidget->addTab(tabIdentify, "  Signal Identification  ");
    m_tabWidget->addTab(tabDemod, "  Demodulation  ");

    // Insert tab widget into main layout (after buttons, before status)
    // Find the index of lblStatus in the layout
    int statusIdx = mainLayout->indexOf(ui.lblStatus);
    if (statusIdx >= 0)
        mainLayout->insertWidget(statusIdx, m_tabWidget, 1);
    else
        mainLayout->addWidget(m_tabWidget, 1);

    // Init log
    ui.textLog->clear();
    setStatus("Ready.");
}

SignalAnalyzerTool::~SignalAnalyzerTool()
{
}

// ====== Create demodulation status widgets ======

void SignalAnalyzerTool::createDemodStatusWidgets()
{
    m_groupBoxDemod = new QGroupBox("Demodulation Status");
    m_groupBoxDemod->setStyleSheet(
        "QGroupBox { color: #ffa726; font-weight: bold; border: 1px solid #555;"
        "  border-radius: 4px; margin-top: 10px; padding-top: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );

    QFormLayout* demodLayout = new QFormLayout(m_groupBoxDemod);

    m_lblDemodLock = new QLabel("-");
    m_lblDemodLock->setStyleSheet("color: #888; font-weight: bold;");
    demodLayout->addRow("Lock:", m_lblDemodLock);

    m_lblDemodFreqOffset = new QLabel("-");
    m_lblDemodFreqOffset->setStyleSheet("color: #d4d4d4;");
    demodLayout->addRow("NCO Freq:", m_lblDemodFreqOffset);

    m_lblDemodSymbolRate = new QLabel("-");
    m_lblDemodSymbolRate->setStyleSheet("color: #d4d4d4;");
    demodLayout->addRow("Symbol Rate:", m_lblDemodSymbolRate);

    m_lblDemodSymbols = new QLabel("-");
    m_lblDemodSymbols->setStyleSheet("color: #d4d4d4;");
    demodLayout->addRow("Symbols:", m_lblDemodSymbols);

    m_lblDemodTime = new QLabel("-");
    m_lblDemodTime->setStyleSheet("color: #d4d4d4;");
    demodLayout->addRow("Time:", m_lblDemodTime);
}

void SignalAnalyzerTool::updateDemodStatus(bool locked, unsigned int symbols,
                                            float freqOffset, float symbolRate,
                                            qint64 elapsedMs)
{
    if (locked) {
        m_lblDemodLock->setText("Locked");
        m_lblDemodLock->setStyleSheet("color: #4caf50; font-weight: bold;");
    } else {
        m_lblDemodLock->setText("Unlocked");
        m_lblDemodLock->setStyleSheet("color: #f44336; font-weight: bold;");
    }
    m_lblDemodFreqOffset->setText(QString::number(freqOffset, 'f', 1) + " Hz");
    m_lblDemodSymbolRate->setText(QString::number(symbolRate, 'f', 1) + " sym/s");
    m_lblDemodSymbols->setText(QString::number(symbols));
    m_lblDemodTime->setText(QString::number(elapsedMs) + " ms");
}

// ====== Browse ======

void SignalAnalyzerTool::onBrowseFile()
{
    QString defaultPath = QFileInfo(m_filePath).exists() ?
        QFileInfo(m_filePath).absolutePath() : QDir::currentPath();
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select IQ Data File", defaultPath,
        "Data Files (*.dat *.bin *.raw);;All Files (*)"
    );
    if (!filePath.isEmpty()) {
        m_filePath = filePath;
        ui.lineEditFile->setText(filePath);
        loadFile(filePath);
    }
}

// ====== Load file ======

void SignalAnalyzerTool::loadFile(const QString& filePath)
{
    setStatus("Loading file...");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Cannot open file: " + filePath);
        setStatus("Error: Cannot open file");
        return;
    }

    QByteArray rawData = file.readAll();
    file.close();

    int numInt16 = rawData.size() / sizeof(int16_t);
    const int16_t* int16Data = reinterpret_cast<const int16_t*>(rawData.constData());

    m_iqData.resize(numInt16);
    for (int i = 0; i < numInt16; i++)
        m_iqData[i] = static_cast<float>(int16Data[i]) / 32768.0f;

    m_sampleRate = static_cast<float>(ui.spinSampleRate->value());
    m_isComplex = (ui.comboSignalType->currentIndex() == 0);

    int numSamples = m_isComplex ? (numInt16 / 2) : numInt16;
    QString info = QString("Loaded %1 samples (%2, %3)")
        .arg(numSamples)
        .arg(m_isComplex ? "Complex IQ" : "Real")
        .arg(QString::number(m_sampleRate / 1e6, 'f', 2) + " MHz");
    setStatus(info);

    m_tabWidget->setCurrentIndex(0);  // show identification tab
    plotPSD();
}

// ====== Analyze ======

void SignalAnalyzerTool::onAnalyze()
{
    if (m_iqData.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please load a data file first.");
        return;
    }

    ui.textLog->clear();
    appendLog("=== Analyzing Signal ===");

    m_sampleRate = static_cast<float>(ui.spinSampleRate->value());
    m_isComplex = (ui.comboSignalType->currentIndex() == 0);

    appendLog(QString("File: %1").arg(m_filePath));
    appendLog(QString("Sample rate: %1 Hz").arg(m_sampleRate));
    appendLog(QString("Signal type: %1").arg(m_isComplex ? "Complex IQ" : "Real"));
    appendLog(QString("Total samples: %1").arg(m_iqData.size()));
    appendLog("---");

    setStatus("Analyzing signal...");
    QApplication::processEvents();

    QElapsedTimer timer;
    timer.start();

    const int MAX_ANALYZE = 65536;
    int ret = -1;

    try {
        if (m_isComplex) {
            int numComplex = m_iqData.size() / 2;
            int useSamples = qMin(numComplex, MAX_ANALYZE);
            QVector<ComplexFloat> iq(useSamples);
            for (int i = 0; i < useSamples; i++) {
                iq[i].real = m_iqData[2 * i];
                iq[i].imag = m_iqData[2 * i + 1];
            }
            ret = liquid_signal_identify_cf32(iq.constData(), useSamples, m_sampleRate, &m_lastInfo);
        } else {
            int useSamples = qMin((int)m_iqData.size(), MAX_ANALYZE);
            ret = liquid_signal_identify_rf32(m_iqData.constData(), useSamples,
                                               m_sampleRate, &m_lastInfo);
        }
    } catch (...) {
        appendLog("ERROR: Exception caught!");
        ret = -1;
    }

    qint64 elapsed = timer.elapsed();

    if (ret == 0) {
        m_hasResults = true;
        updateResults(m_lastInfo);
        ui.btnDemod->setEnabled(true);

        appendLog("Center freq:  " + QString::number(m_lastInfo.center_freq, 'f', 1) + " Hz");
        appendLog("Symbol rate:  " + QString::number(m_lastInfo.symbol_rate, 'f', 1) + " sym/s");
        appendLog("Bandwidth:    " + QString::number(m_lastInfo.bandwidth, 'f', 1) + " Hz");
        appendLog("SNR:          " + QString::number(m_lastInfo.snr, 'f', 1) + " dB");
        appendLog("Modulation:   " + modTypeName(m_lastInfo.modulation_type));
        appendLog("Time:         " + QString::number(elapsed) + " ms");
    } else {
        appendLog("Analysis failed");
    }

    // Update PSD and waterfall
    plotPSD();
    plotWaterfall();

    m_tabWidget->setCurrentIndex(0);
    setStatus(QString("Done in %1 ms").arg(elapsed));
}

// ====== Demodulate ======

void SignalAnalyzerTool::onDemodulate()
{
    if (m_iqData.isEmpty() || !m_hasResults) {
        QMessageBox::warning(this, "Warning", "Please analyze a signal first.");
        return;
    }

    setStatus("Demodulating...");
    appendLog("=== Demodulating ===");
    QApplication::processEvents();
    QElapsedTimer timer;
    timer.start();

    int modType = m_lastInfo.modulation_type;
    if (ui.comboModType->currentIndex() > 0)
        modType = modTypeFromName(ui.comboModType->currentText());

    float symbolRate = static_cast<float>(ui.spinSymbolRate->value());
    if (ui.comboModType->currentIndex() == 0)
        symbolRate = m_lastInfo.symbol_rate;

    void* demod = liquid_demod_create(modType, symbolRate, m_sampleRate);
    if (!demod) { setStatus("Error: Failed to create demodulator"); return; }

    // Build complex buffer
    QVector<ComplexFloat> iq;
    if (m_isComplex) {
        int nc = m_iqData.size() / 2;
        iq.resize(nc);
        for (int i = 0; i < nc; i++) {
            iq[i].real = m_iqData[2 * i];
            iq[i].imag = m_iqData[2 * i + 1];
        }
    } else {
        iq.resize(m_iqData.size());
        for (int i = 0; i < m_iqData.size(); i++) {
            iq[i].real = m_iqData[i];
            iq[i].imag = 0.0f;
        }
    }
    liquid_demod_set_center_freq(demod, m_lastInfo.center_freq);

    // Block processing
    const int BLOCK_SIZE = 65536;
    QVector<unsigned int> allSymbols;
    allSymbols.reserve(iq.size() / 10);
    uint64_t totalSymbols = 0;
    int totalProcessed = 0;
    int totalSamples = iq.size();

    m_tabWidget->setCurrentIndex(1);  // switch to demod tab

    while (totalProcessed < totalSamples) {
        int bs = qMin(BLOCK_SIZE, totalSamples - totalProcessed);
        QVector<unsigned int> blockSymbols(bs);
        unsigned int blockNumSymbols = 0;

        int ret = liquid_demod_execute(demod, iq.constData() + totalProcessed,
                                        bs, blockSymbols.data(), &blockNumSymbols);
        if (ret != 0) {
            liquid_demod_destroy(demod);
            setStatus("Demodulation failed");
            return;
        }
        for (unsigned int i = 0; i < blockNumSymbols; i++)
            allSymbols.append(blockSymbols[i]);
        totalSymbols += blockNumSymbols;
        totalProcessed += bs;

        // Build constellation from soft symbols (matched filter output before decision)
        const unsigned int MAX_SOFT = 4096;
        ComplexFloat softBuf[MAX_SOFT];
        unsigned int softCount = MAX_SOFT;
        liquid_demod_get_soft_symbols(demod, softBuf, &softCount);
        QVector<ComplexFloat> constellation;
        if (softCount > 0) {
            unsigned int step = softCount / qMin(softCount, 4096u);
            if (step < 1) step = 1;
            for (unsigned int i = 0; i < softCount && constellation.size() < 4096; i += step)
                constellation.append(softBuf[i]);
        }
        plotConstellation(constellation);
        updateDemodStatus(totalSymbols > 0, (unsigned int)allSymbols.size(),
                          m_lastInfo.center_freq, symbolRate, timer.elapsed());
        setStatus(QString("Demodulating... %1% (%2 symbols)")
                  .arg(totalProcessed * 100 / totalSamples).arg(totalSymbols));
        QApplication::processEvents();
    }

    liquid_demod_destroy(demod);
    
    // Save demodulated bits to file for verification
    {
        QFile saveFile(QFileInfo(m_filePath).absolutePath() + "/demod_out.dat");
        if (saveFile.open(QIODevice::WriteOnly)) {
            for (unsigned int i = 0; i < (unsigned int)allSymbols.size(); i++) {
                // Format: same as reference file
                // bit0 = I hard decision (0 or 1)
                // bit4 = Q hard decision (0 for BPSK)
                unsigned short val = 0;
                if (allSymbols[i] != 0) val |= 0x0001;      // bit 0
                // Check Q from soft symbols for bit4
                if (i < (unsigned int)allSymbols.size()) val |= 0x0000;  // Q=0 for BPSK
                saveFile.write((const char*)&val, sizeof(val));
            }
            saveFile.close();
            appendLog(QString("Saved %1 symbols to demod_out.dat").arg(allSymbols.size()));
        }
    }
    
    updateDemodStatus(totalSymbols > 0, (unsigned int)allSymbols.size(),
                      m_lastInfo.center_freq, symbolRate, timer.elapsed());

    qint64 elapsed = timer.elapsed();
    appendLog(QString("Demodulated %1 symbols in %2 ms").arg(totalSymbols).arg(elapsed));
    setStatus(QString("Done: %1 symbols in %2 ms").arg(totalSymbols).arg(elapsed));
}

// ====== Clear ======

void SignalAnalyzerTool::onClear()
{
    m_iqData.clear();
    m_hasResults = false;
    ui.lineEditFile->clear();
    ui.lblCenterFreq->setText("-");
    ui.lblSymbolRate->setText("-");
    ui.lblBandwidth->setText("-");
    ui.lblSNR->setText("-");
    ui.lblModType->setText("-");
    ui.lblModOrder->setText("-");
    ui.btnDemod->setEnabled(false);

    m_lblDemodLock->setText("-");
    m_lblDemodLock->setStyleSheet("color: #888; font-weight: bold;");
    m_lblDemodFreqOffset->setText("-");
    m_lblDemodSymbolRate->setText("-");
    m_lblDemodSymbols->setText("-");
    m_lblDemodTime->setText("-");

    m_plotPSD->graph(0)->data()->clear();
    m_plotPSD->replot();
    m_plotConstellation->graph(0)->data()->clear();
    m_plotConstellation->xAxis->setRange(-2, 2);
    m_plotConstellation->yAxis->setRange(-2, 2);
    m_plotConstellation->replot();

    // Clear waterfall
    m_plotWaterfall->clearPlottables();
    m_waterfallData.clear();
    m_plotWaterfall->replot();

    m_tabWidget->setCurrentIndex(0);
    setStatus("Cleared.");
}

// ====== PSD plot ======

void SignalAnalyzerTool::plotPSD()
{
    if (m_iqData.isEmpty()) return;

    int fftSize = 2048;
    int numSamples = m_iqData.size();
    if (numSamples < fftSize) fftSize = numSamples;
    if (fftSize < 64) fftSize = 64;

    void* fft = liquid_fft_create(fftSize);
    if (!fft) return;

    // Average multiple FFTs
    int numAvgs = numSamples / fftSize;
    if (numAvgs < 1) numAvgs = 1;
    QVector<double> psdSum(fftSize, 0.0);

    for (int avg = 0; avg < numAvgs; avg++) {
        QVector<ComplexFloat> buf(fftSize);
        int offset = avg * fftSize;
        for (int i = 0; i < fftSize; i++) {
            double w = 0.5 * (1.0 - cos(2.0 * M_PI * i / (fftSize - 1)));
            if (i + offset < numSamples) {
                if (m_isComplex && 2 * (i + offset) + 1 < numSamples) {
                    buf[i].real = (float)(m_iqData[2 * (i + offset)] * w);
                    buf[i].imag = (float)(m_iqData[2 * (i + offset) + 1] * w);
                } else {
                    buf[i].real = (float)(m_iqData[i + offset] * w);
                    buf[i].imag = 0.0f;
                }
            }
        }
        liquid_fft_execute_forward(fft, buf.data(), buf.data());
        for (int i = 0; i < fftSize; i++)
            psdSum[i] += buf[i].real * buf[i].real + buf[i].imag * buf[i].imag;
    }
    liquid_fft_destroy(fft);

    // Build PSD data (positive half only: 0 ~ fs/2)
    int halfSize = fftSize / 2;
    QVector<double> freq(halfSize);
    QVector<double> psd(halfSize);
    double freqRes = m_sampleRate / fftSize;
    double maxPsd = -1e100;

    for (int i = 0; i < halfSize; i++) {
        psd[i] = 10.0 * log10(psdSum[i + halfSize] / numAvgs + 1e-20);
        if (psd[i] > maxPsd) maxPsd = psd[i];
        freq[i] = i * freqRes;
    }

    m_plotPSD->graph(0)->setData(freq, psd);
    m_plotPSD->xAxis->setRange(0, m_sampleRate / 2.0);
    m_plotPSD->yAxis->setRange(maxPsd - 60, maxPsd + 5);
    m_plotPSD->replot();

    // Sync waterfall x-axis to PSD
    if (m_plotWaterfall->xAxis) {
        m_plotWaterfall->xAxis->setRange(m_plotPSD->xAxis->range());
        m_plotWaterfall->replot();
    }
}

// ====== Waterfall / Spectrogram ======

void SignalAnalyzerTool::plotWaterfall()
{
    if (m_iqData.isEmpty()) return;

    int fftSize = 512;
    int hop = fftSize / 2;  // 50% overlap
    int numSamples = m_iqData.size();
    int numRows = qMin(200, (numSamples - fftSize) / hop);  // limit rows for performance
    if (numRows < 1) return;

    void* fft = liquid_fft_create(fftSize);

    // Build waterfall data using positive half (0 ~ fs/2)
    m_waterfallData.clear();
    m_waterfallData.resize(numRows);
    int halfSize = fftSize / 2;
    for (int row = 0; row < numRows; row++) {
        m_waterfallData[row].resize(halfSize);
        int offset = row * hop;
        QVector<ComplexFloat> buf(fftSize);
        for (int i = 0; i < fftSize; i++) {
            double w = 0.5 * (1.0 - cos(2.0 * M_PI * i / (fftSize - 1)));
            if (i + offset < numSamples) {
                if (m_isComplex && 2 * (i + offset) + 1 < numSamples) {
                    buf[i].real = (float)(m_iqData[2 * (i + offset)] * w);
                    buf[i].imag = (float)(m_iqData[2 * (i + offset) + 1] * w);
                } else {
                    buf[i].real = (float)(m_iqData[i + offset] * w);
                    buf[i].imag = 0.0f;
                }
            }
        }
        liquid_fft_execute_forward(fft, buf.data(), buf.data());
        for (int i = 0; i < halfSize; i++) {
            int idx = i + halfSize;  // positive frequencies (0 ~ fs/2)
            m_waterfallData[row][i] = 10.0 * log10(
                buf[idx].real * buf[idx].real + buf[idx].imag * buf[idx].imag + 1e-20);
        }
    }
    liquid_fft_destroy(fft);

    // Create the color map (positive half only)
    m_plotWaterfall->clearPlottables();
    QCPColorMap* colorMap = new QCPColorMap(m_plotWaterfall->xAxis, m_plotWaterfall->yAxis);
    colorMap->data()->setSize(halfSize, numRows);
    colorMap->data()->setRange(QCPRange(0, m_sampleRate / 2),
                               QCPRange(0, numRows * hop / m_sampleRate));

    for (int row = 0; row < numRows; row++)
        for (int i = 0; i < halfSize; i++)
            colorMap->data()->setCell(i, row, m_waterfallData[row][i]);

    // Color gradient
    QCPColorGradient gradient(QCPColorGradient::gpJet);
    colorMap->setGradient(gradient);
    colorMap->rescaleDataRange(true);

    m_plotWaterfall->xAxis->setLabel("Frequency (Hz)");
    m_plotWaterfall->yAxis->setLabel("Time (s)");
    // Sync x-axis range with PSD
    m_plotWaterfall->xAxis->setRange(m_plotPSD->xAxis->range());
    m_plotWaterfall->replot();
}

// ====== Constellation ======

void SignalAnalyzerTool::plotConstellation(const QVector<ComplexFloat>& points)
{
    if (points.isEmpty()) {
        m_plotConstellation->graph(0)->data()->clear();
        m_plotConstellation->xAxis->setRange(-1, 1);
        m_plotConstellation->yAxis->setRange(-1, 1);
        m_plotConstellation->replot();
        return;
    }

    // Normalize to unit power, then scale so constellation fits in [-1, 1]
    double pwr = 0.0;
    for (const auto& p : points)
        pwr += p.real * (double)p.real + p.imag * (double)p.imag;
    pwr /= points.size();
    double scale = 1.0 / sqrt(pwr + 1e-20);

    QVector<double> iVals(points.size());
    QVector<double> qVals(points.size());
    for (int i = 0; i < points.size(); i++) {
        iVals[i] = points[i].real * scale;
        qVals[i] = points[i].imag * scale;
    }

    m_plotConstellation->graph(0)->setData(iVals, qVals);
    m_plotConstellation->xAxis->setRange(-1, 1);
    m_plotConstellation->yAxis->setRange(-1, 1);
    m_plotConstellation->replot();
    ensureSquareConstellation();
}

// ====== Square constellation ======

void SignalAnalyzerTool::ensureSquareConstellation()
{
    double w = static_cast<double>(m_plotConstellation->axisRect()->width());
    double h = static_cast<double>(m_plotConstellation->axisRect()->height());
    if (w < 1.0 || h < 1.0) return;
    double xRange = m_plotConstellation->xAxis->range().size();
    double yRange = m_plotConstellation->yAxis->range().size();
    if (xRange < 1e-10 || yRange < 1e-10) return;
    double newXRange = yRange * (w / h);
    double cx = m_plotConstellation->xAxis->range().center();
    m_plotConstellation->xAxis->setRange(cx - newXRange * 0.5, cx + newXRange * 0.5);
}

// ====== Update results ======

void SignalAnalyzerTool::updateResults(const SignalInfo& info)
{
    ui.lblCenterFreq->setText(QString::number(info.center_freq, 'f', 1) + " Hz");
    ui.lblSymbolRate->setText(QString::number(info.symbol_rate, 'f', 1) + " sym/s");
    ui.lblBandwidth->setText(QString::number(info.bandwidth, 'f', 1) + " Hz");
    ui.lblSNR->setText(QString::number(info.snr, 'f', 1) + " dB");
    ui.lblModType->setText(modTypeName(info.modulation_type));
    ui.lblModOrder->setText(QString::number(info.modulation_order));
    if (info.symbol_rate > 0) ui.spinSymbolRate->setValue(info.symbol_rate);
    ui.comboModType->setCurrentIndex(0);
}

// ====== Helpers ======

QString SignalAnalyzerTool::modTypeName(int type)
{
    switch (type) {
        case LIQUID_MODEM_BPSK:   return "BPSK";
        case LIQUID_MODEM_QPSK:   return "QPSK";
        case LIQUID_MODEM_8PSK:   return "8PSK";
        case LIQUID_MODEM_16PSK:  return "16PSK";
        case LIQUID_MODEM_16QAM:  return "16QAM";
        case LIQUID_MODEM_32QAM:  return "32QAM";
        case LIQUID_MODEM_64QAM:  return "64QAM";
        case LIQUID_MODEM_256QAM: return "256QAM";
        case LIQUID_MODEM_ASK:    return "ASK";
        case LIQUID_MODEM_PSK:    return "PSK";
        case LIQUID_MODEM_QAM:    return "QAM";
        case LIQUID_MODEM_APSK:   return "APSK";
        case LIQUID_MODEM_FSK:    return "FSK";
        case LIQUID_MODEM_GMSK:   return "GMSK";
        default: return QString("Unknown (%1)").arg(type);
    }
}

int SignalAnalyzerTool::modTypeFromName(const QString& name)
{
    if (name == "BPSK")   return LIQUID_MODEM_BPSK;
    if (name == "QPSK")   return LIQUID_MODEM_QPSK;
    if (name == "8PSK")   return LIQUID_MODEM_8PSK;
    if (name == "16QAM")  return LIQUID_MODEM_16QAM;
    if (name == "32QAM")  return LIQUID_MODEM_32QAM;
    if (name == "64QAM")  return LIQUID_MODEM_64QAM;
    if (name == "256QAM") return LIQUID_MODEM_256QAM;
    return LIQUID_MODEM_QPSK;
}

void SignalAnalyzerTool::setStatus(const QString& msg)
{
    ui.lblStatus->setText(msg);
    statusBar()->showMessage(msg, 5000);
}

void SignalAnalyzerTool::appendLog(const QString& msg)
{
    ui.textLog->append(msg);
}
