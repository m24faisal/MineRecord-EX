#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "programinfo.h"
#include "processdetector.h"
#include "infodialog.h"
#include "settingsdialog.h"
#include "pythonbackendwrapper.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QEvent>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QStyleFactory>
#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>
#include <QPalette>
#include <QScrollBar>
#include <QUuid>
#include <QScopedValueRollback>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_pythonWrapper(nullptr),
    executableTable(nullptr),
    centralWidget(nullptr),
    updateTimer(nullptr),
    programs(),
    contextMenu(nullptr),
    removeAction(nullptr),
    startRecordingAction(nullptr),
    stopRecordingAction(nullptr),
    infoDialog(nullptr),
    settingsDialog(nullptr),
    activeRecordingId(""),
    enableDataCollection(false),
    m_lastSelectedExecutablePath(""),
    m_isRecordingActionInProgress(false)
{
    ui->setupUi(this);

    // Set Application Icon
    setWindowIcon(QIcon(":/app_icon.ico"));

    setupUI();

    // Ensure backend directory and default db_config.json exist
    QString backendDir = QCoreApplication::applicationDirPath() + "/backend";
    QDir().mkpath(backendDir);
    QString configPath = backendDir + "/db_config.json";
    if (!QFile::exists(configPath)) {
        QJsonObject dbConfig;
        dbConfig["host"] = "127.0.0.1";
        dbConfig["port"] = "5432";
        dbConfig["database"] = "playerdata";
        dbConfig["user"] = "postgres";
        dbConfig["password"] = "postgres";

        QJsonDocument doc(dbConfig);
        QFile configFile(configPath);
        if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            configFile.write(doc.toJson());
            configFile.close();
            qDebug() << "Created default db_config.json at:" << configPath;
        }
    }

    loadSettings();
    applySettings();

    // Set up timers
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateProgramStatus);
    updateTimer->start(1000);

    ProcessDetector::instance().setUpdateInterval(500);
    connect(&ProcessDetector::instance(), &ProcessDetector::processListUpdated,
            this, &MainWindow::updateProgramStatus);

    // Context menu setup
    contextMenu = new QMenu(this);
    removeAction = new QAction("Remove Game", this);
    startRecordingAction = new QAction("Start Recording", this);
    stopRecordingAction = new QAction("Stop Recording", this);
    contextMenu->addActions({removeAction, startRecordingAction, stopRecordingAction});

    // Context menu styling
    QSettings settings("Stat Tracker", "MineRecordEX");
    QString currentTheme = settings.value("theme", "Light").toString();

    if (currentTheme == "Dark") {
        contextMenu->setStyleSheet(R"(
            QMenu {
                background-color: #2A2A2A;
                border: 1px solid #404040;
            }
            QMenu::item {
                padding: 6px 20px 6px 20px;
                color: white;
            }
            QMenu::item:selected {
                background-color: #454545;
                color: white;
            }
            QMenu::item:disabled {
                color: #777777;
            }
        )");
    } else {
        contextMenu->setStyleSheet(R"(
            QMenu {
                background-color: white;
                border: 1px solid #CCCCCC;
            }
            QMenu::item {
                padding: 6px 20px 6px 20px;
                color: black;
            }
            QMenu::item:selected {
                background-color: #E0E0E0;
                color: black;
            }
            QMenu::item:disabled {
                color: #999999;
            }
        )");
    }

    executableTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(executableTable, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    // SAFE RECORDING ACTION CONNECTIONS (NO AUTO-CONNECTION RISK)
    connect(ui->fileActionStartRecording, &QAction::triggered, this, [this]() {
        static bool inProgress = false;
        if (!inProgress) {
            inProgress = true;
            startRecording();
            inProgress = false;
        }
    });

    connect(ui->fileActionStopRecording, &QAction::triggered, this, [this]() {
        static bool inProgress = false;
        if (!inProgress) {
            inProgress = true;
            stopRecording();
            inProgress = false;
        }
    });

    // Context menu actions
    connect(startRecordingAction, &QAction::triggered, this, &MainWindow::startRecording);
    connect(stopRecordingAction, &QAction::triggered, this, &MainWindow::stopRecording);
    connect(removeAction, &QAction::triggered, this, &MainWindow::removeGame);

    // Preserve table selection behavior
    executableTable->setFocusPolicy(Qt::StrongFocus);
    executableTable->setSelectionMode(QAbstractItemView::SingleSelection);
    executableTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    executableTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // CRITICAL FIX: Install event filter ONLY on central widget (NOT on qApp or table)
    // Install event filter on the main window itself
    // Replace: centralWidget->installEventFilter(this);
    // With this:
    executableTable->viewport()->installEventFilter(this);

    // Load program data
    loadProgramData();

    if (executableTable) {
        executableTable->clearSelection();
        executableTable->setCurrentCell(-1, -1);
    }

    // Python backend
    m_pythonWrapper = new PythonBackendWrapper();
    m_pythonWrapper->initialize();
    if (enableDataCollection) {
        m_pythonWrapper->startDataService(); // ← Data service starts with application
    }

    // Connect cleanup before quit
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::cleanupBeforeQuit);
}

MainWindow::~MainWindow()
{
    // Cleanup is handled by cleanupBeforeQuit()
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    executableTable = new QTableWidget(this);
    executableTable->setColumnCount(4);
    executableTable->setHorizontalHeaderLabels(QStringList() << "Program Name" << "Running" << "Time Played" << "Recording");
    executableTable->horizontalHeader()->setStretchLastSection(true);
    executableTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    executableTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    executableTable->setSelectionMode(QAbstractItemView::SingleSelection);
    executableTable->setAlternatingRowColors(true);
    executableTable->setFocusPolicy(Qt::StrongFocus); // Enable focus tracking

    layout->addWidget(executableTable);
    setCentralWidget(centralWidget);
    setWindowTitle("MineRecordEX");
    resize(800, 600);
}

void MainWindow::onAppFocusChanged(QWidget *old, QWidget *now)
{
    if (executableTable && old == executableTable && now != executableTable) {
        executableTable->clearSelection();
        executableTable->setCurrentCell(-1, -1);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Check if any game is currently being recorded
    bool hasActiveRecording = false;
    const QSet<QString>& recordingPaths = activeRecordingPaths; // ← Const reference to avoid detachment
    for (const QString& path : recordingPaths) {
        if (programs.contains(path)) {
            hasActiveRecording = true;
            break;
        }
    }

    if (hasActiveRecording) {
        QMessageBox::information(this, "Stopping Recordings",
                                 "Active recordings detected. Stopping and saving...");

        // Stop all active recordings via Python backend
        const QSet<QString>& recordingPaths2 = activeRecordingPaths; // ← Const reference to avoid detachment
        for (const QString& path : recordingPaths2) {
            if (programs.contains(path)) {
                ProgramInfo *program = programs[path];
                QString result = stopPythonRecording(program->recordingId());
                qDebug() << "Stopped recording for:" << program->name() << "Result:" << result;
                program->setRecording(false);
            }
        }
        activeRecordingPaths.clear();
        saveProgramData(); // Update UI state (optional but clean)
    }

    // Proceed with normal close
    QMainWindow::closeEvent(event);
}

// --- Python Backend Methods (Updated to use wrapper) ---
void MainWindow::initializePythonBackend()
{
    if (m_pythonWrapper) {
        qDebug() << "SUCCESS: Python backend initialized via wrapper.";
    } else {
        qCritical() << "FAILURE: Python backend wrapper not created.";
    }
}

void MainWindow::shutdownPythonBackend()
{
    // Handled in cleanupBeforeQuit()
}

QString MainWindow::startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath)
{
    if (!m_pythonWrapper) return "Error: Python wrapper is null.";
    std::string result = m_pythonWrapper->startRecording(
        gameName.toStdString(),
        gamePath.toStdString(),
        recordingPath.toStdString(),
        enableDataCollection
        );
    return QString::fromStdString(result);
}

QString MainWindow::stopPythonRecording(const QString &recordingId)
{
    if (!m_pythonWrapper) return "Error: Python wrapper is null.";
    std::string result = m_pythonWrapper->stopRecording(recordingId.toStdString());
    return QString::fromStdString(result);
}

// --- NEW: Required cleanup slot ---
void MainWindow::cleanupBeforeQuit()
{
    qDebug() << "Stopping global Python data collection service...";
    if (m_pythonWrapper) {
        m_pythonWrapper->stopDataService(); // ← Data service stops only on application exit
        m_pythonWrapper->shutdown();
        delete m_pythonWrapper;
        m_pythonWrapper = nullptr;
    }

    // Stop all FFmpeg processes
    for (auto it = m_activeProcesses.begin(); it != m_activeProcesses.end(); ++it) {
        QProcess *process = it.value();
        if (process && process->state() == QProcess::Running) {
            process->terminate();
            if (!process->waitForFinished(2000)) {
                process->kill();
                process->waitForFinished(1000);
            }
            delete process;
        }
    }
    m_activeProcesses.clear();

    saveProgramData();
}

// --- Recording Slots ---
void MainWindow::startRecording()
{
    if (!executableTable) {
        qDebug() << "Executable table is null";
        return;
    }

    QList<QTableWidgetSelectionRange> selection = executableTable->selectedRanges();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a game from the table first.");
        return;
    }

    int row = selection.first().topRow();
    if (row < 0 || row >= executableTable->rowCount()) {
        qDebug() << "Invalid row selection:" << row;
        return;
    }

    QTableWidgetItem *nameItem = executableTable->item(row, 0);
    if (!nameItem) {
        qDebug() << "Missing name item at row" << row;
        QMessageBox::critical(this, "Error", "Invalid game entry selected.");
        return;
    }

    QString executablePath = nameItem->data(Qt::UserRole).toString();
    QString gameName = nameItem->text();

    if (executablePath.isEmpty()) {
        qDebug() << "Empty executable path for game:" << gameName;
        QMessageBox::critical(this, "Error", "Selected game has no executable path.");
        return;
    }

    if (!activeRecordingId.isEmpty()) {
        QMessageBox::warning(this, "Already Recording", "A recording is already in progress.");
        return;
    }

    QFileInfo fileInfo(executablePath);
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        QMessageBox::critical(this, "File Not Found",
                              QString("Executable not found or not accessible:\n%1").arg(executablePath));
        return;
    }

    // RESTORE THIS LINE:
    activeRecordingId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QSettings settings("Stat Tracker", "MineRecordEX");
    QString recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();
    QDir().mkpath(recordingPath);

    // Update ProgramInfo BEFORE starting process
    QString path = nameItem->data(Qt::UserRole).toString();
    if (programs.contains(path)) {
        programs[path]->setRecording(true);
    }
    activeRecordingPaths.insert(path);
    saveProgramData();

    bool success = startRecordingProcess(executablePath, recordingPath, gameName);

    if (success) {
        // Update only the specific row
        QTableWidgetItem *recordingItem = executableTable->item(row, 3);
        if (recordingItem) {
            recordingItem->setText("Yes");
            recordingItem->setBackground(QBrush(QColor(255, 165, 0)));
        }

        QMessageBox::information(this, "Recording Started",
                                 QString("Recording started for %1").arg(gameName));
    } else {
        activeRecordingPaths.remove(path);
        activeRecordingId.clear(); // Clear on failure
        // Revert ProgramInfo state
        if (programs.contains(path)) {
            programs[path]->setRecording(false);
        }
        saveProgramData();

        QMessageBox::critical(this, "Recording Failed",
                              "Failed to start recording process.");
    }
}

void MainWindow::stopRecording()
{
    if (!executableTable) {
        qDebug() << "Executable table is null";
        return;
    }

    QList<QTableWidgetSelectionRange> selection = executableTable->selectedRanges();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a game from the table first.");
        return;
    }

    int row = selection.first().topRow();
    if (row < 0 || row >= executableTable->rowCount()) {
        qDebug() << "Invalid row selection:" << row;
        return;
    }

    QTableWidgetItem *nameItem = executableTable->item(row, 0);
    if (!nameItem) {
        qDebug() << "Missing name item at row" << row;
        QMessageBox::critical(this, "Error", "Invalid game entry selected.");
        return;
    }

    QString gameName = nameItem->text();

    if (activeRecordingId.isEmpty()) {
        QMessageBox::warning(this, "Not Recording", "No active recording to stop.");
        return;
    }

    // Update ProgramInfo BEFORE stopping process
    QString path = nameItem->data(Qt::UserRole).toString();
    if (programs.contains(path)) {
        programs[path]->setRecording(false);
    }
    activeRecordingPaths.remove(path);
    saveProgramData();

    bool success = stopRecordingProcess(gameName);

    if (success) {
        // Update only the specific row
        QTableWidgetItem *recordingItem = executableTable->item(row, 3);
        if (recordingItem) {
            recordingItem->setText("No");
            recordingItem->setBackground(Qt::NoBrush);
        }

        activeRecordingId.clear(); // CRITICAL: Clear the recording ID
        QMessageBox::information(this, "Recording Stopped",
                                 QString("Recording stopped for %1").arg(gameName));
    } else {
        QMessageBox::warning(this, "Stop Failed",
                             "Failed to stop recording process.");
    }
}

bool MainWindow::startRecordingProcess(const QString &executablePath, const QString &recordingDir, const QString &gameName)
{
    // Launch game
    QProcess *gameProcess = new QProcess(this);
    gameProcess->setWorkingDirectory(QFileInfo(executablePath).absolutePath());
    gameProcess->start(executablePath);

    if (!gameProcess->waitForStarted(5000)) {
        qDebug() << "Failed to start game process:" << gameProcess->errorString();
        delete gameProcess;
        return false;
    }

    // Launch FFmpeg for screen recording WITH mouse cursor
    QProcess *ffmpegProcess = new QProcess(this);
    QString videoPath = recordingDir + "/" + gameName + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mp4";

    QStringList ffmpegArgs;
    ffmpegArgs << "-f" << "gdigrab"
               << "-thread_queue_size" << "512"
               << "-framerate" << "30"
               << "-probesize" << "10M"
               << "-i" << "desktop"
               << "-draw_mouse" << "1"
               << "-c:v" << "libx264"
               << "-preset" << "ultrafast"
               << "-crf" << "23"
               << "-pix_fmt" << "yuv420p"
               << "-y"
               << videoPath;

    ffmpegProcess->start("ffmpeg", ffmpegArgs);

    if (!ffmpegProcess->waitForStarted(5000)) {
        qDebug() << "Failed to start FFmpeg:" << ffmpegProcess->errorString();
        gameProcess->terminate();
        delete gameProcess;
        delete ffmpegProcess;
        return false;
    }

    // Store both processes for later cleanup
    m_activeProcesses[gameName] = gameProcess;
    m_activeProcesses[gameName + "_ffmpeg"] = ffmpegProcess;

    qDebug() << "Started recording for:" << gameName << "at" << recordingDir;
    return true;
}

bool MainWindow::stopRecordingProcess(const QString &gameName)
{
    // Stop FFmpeg process
    QString ffmpegKey = gameName + "_ffmpeg";
    if (m_activeProcesses.contains(ffmpegKey)) {
        QProcess *ffmpegProcess = m_activeProcesses[ffmpegKey];
        if (ffmpegProcess && ffmpegProcess->state() == QProcess::Running) {
            ffmpegProcess->write("q"); // Send 'q' to gracefully quit FFmpeg
            ffmpegProcess->waitForFinished(5000);
            if (ffmpegProcess->state() == QProcess::Running) {
                ffmpegProcess->kill();
                ffmpegProcess->waitForFinished(1000);
            }
            delete ffmpegProcess;
            m_activeProcesses.remove(ffmpegKey);
        }
    }

    // Stop game process
    if (m_activeProcesses.contains(gameName)) {
        QProcess *gameProcess = m_activeProcesses[gameName];
        if (gameProcess && gameProcess->state() == QProcess::Running) {
            gameProcess->terminate();
            gameProcess->waitForFinished(2000);
            if (gameProcess->state() == QProcess::Running) {
                gameProcess->kill();
                gameProcess->waitForFinished(1000);
            }
            delete gameProcess;
            m_activeProcesses.remove(gameName);
        }
    }

    // REMOVED: Data service stop call from recording stop
    // Data service continues running until application exit

    qDebug() << "Stopped recording for:" << gameName;
    return true;
}

void MainWindow::handleDataCollectionError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "Data collection process failed to start.";
        break;
    case QProcess::Crashed:
        errorMsg = "Data collection process crashed.";
        break;
    case QProcess::Timedout:
        errorMsg = "Data collection process timed out.";
        break;
    case QProcess::WriteError:
        errorMsg = "Error writing to data collection process.";
        break;
    case QProcess::ReadError:
        errorMsg = "Error reading from data collection process.";
        break;
    default:
        errorMsg = "Unknown error occurred with data collection process.";
        break;
    }
    QMessageBox::critical(this, "Data Collection Error", errorMsg);
}

// --- All other existing methods ---
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // When window is restored (from minimized or maximized), clear any selection
        if (!isMinimized() && isVisible()) {
            if (executableTable) {
                executableTable->clearSelection();
                executableTable->setCurrentCell(-1, -1);
            }
        }
    }
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // If we have a table and it has a selection
        if (executableTable && executableTable->selectedItems().size() > 0) {
            // Get the item at the clicked position
            QPoint pos = mouseEvent->pos();
            QTableWidgetItem *item = executableTable->itemAt(pos);

            // If no item was clicked (empty space in table), deselect
            if (!item) {
                executableTable->clearSelection();
                executableTable->setCurrentCell(-1, -1);
                m_lastSelectedExecutablePath.clear();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_actionAdd_Game_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select Executable",
                                                    QDir::homePath(),
                                                    "Executable Files (*.exe *.bat *.cmd *.app *.sh);;All Files (*)");

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        addExecutableToTable(fileInfo);
    }
}

void MainWindow::on_actionExit_Application_triggered()
{ this->close(); }

void MainWindow::on_actionGitHub_triggered()
{ QDesktopServices::openUrl(QUrl("https://github.com/m24faisal/MineRecord-EX")); }

void MainWindow::on_actionKo_Fi_triggered()
{ QDesktopServices::openUrl(QUrl("https://ko-fi.com/m24faisal")); }

void MainWindow::on_actionInfo_triggered()
{
    if (!infoDialog) infoDialog = new InfoDialog(this);
    infoDialog->show();
    infoDialog->raise();
    infoDialog->activateWindow();
}

void MainWindow::on_actionSettings_triggered()
{
    if (!settingsDialog) {
        settingsDialog = new SettingsDialog(
            this,  // MainWindow* mainWin
            nullptr,  // QWidget* parent (explicitly pass nullptr)
            [this](const QString &playerName, const QString &exportPath) {
                if (!m_pythonWrapper) {
                    return QString("Error: Python wrapper is not initialized.");
                }
                std::string result = m_pythonWrapper->exportPlayerData(playerName.toStdString(), exportPath.toStdString());
                return QString::fromStdString(result);
            }
            );
        // Apply current theme to settings dialog immediately
        settingsDialog->setStyleSheet(getCurrentThemeStyleSheet());
    }
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void MainWindow::updateProgramStatus()
{
    if (!executableTable) return;

    // Save the EXECUTABLE PATH of the selected item (not just row index)
    QString selectedExecutablePath;
    int currentRow = executableTable->currentRow();
    if (currentRow >= 0 && currentRow < executableTable->rowCount()) {
        QTableWidgetItem *nameItem = executableTable->item(currentRow, 0);
        if (nameItem) {
            selectedExecutablePath = nameItem->data(Qt::UserRole).toString();
        }
    }

    // Clear all existing items to remove cached appearance
    executableTable->clearContents();
    executableTable->setRowCount(0);

    // Repopulate with fresh items that respect current palette
    int newRow = 0;
    int targetRow = -1; // Track where the previously selected item ends up

    for (auto it = programs.begin(); it != programs.end(); ++it) {
        QString path = it.key();
        ProgramInfo *program = it.value();

        if (!program) continue;

        bool isRunning = ProcessDetector::instance().isProcessRunning(program->name());
        if (program->isRunning() != isRunning) {
            program->setRunning(isRunning);
        }

        QString timePlayed = program->formattedTimePlayed();
        // Use activeRecordingPaths instead of program->isRecording()
        bool isRecording = activeRecordingPaths.contains(path);  // ← CRITICAL FIX

        // Create FRESH items
        QTableWidgetItem *nameItem = new QTableWidgetItem(program->name());
        nameItem->setData(Qt::UserRole, path);

        QTableWidgetItem *runningItem = new QTableWidgetItem(isRunning ? "Yes" : "No");
        QTableWidgetItem *timeItem = new QTableWidgetItem(timePlayed);
        QTableWidgetItem *recordingItem = new QTableWidgetItem(isRecording ? "Yes" : "No");

        // RESTORE STATUS HIGHLIGHTS
        if (!isRunning) {
            runningItem->setBackground(QBrush(QColor(255, 182, 193))); // Light pink/red for "Not Running"
        } else {
            runningItem->setBackground(QBrush(QColor(144, 238, 144))); // Light green for "Running"
        }

        if (isRecording) {
            recordingItem->setBackground(QBrush(QColor(255, 165, 0))); // Orange for "Recording"
        }

        executableTable->insertRow(newRow);
        executableTable->setItem(newRow, 0, nameItem);
        executableTable->setItem(newRow, 1, runningItem);
        executableTable->setItem(newRow, 2, timeItem);
        executableTable->setItem(newRow, 3, recordingItem);

        // Track where our selected item is
        if (!selectedExecutablePath.isEmpty() && path == selectedExecutablePath) {
            targetRow = newRow;
        }

        newRow++;
    }

    // Restore selection using EXECUTABLE PATH match (not row index)
    if (targetRow >= 0) {
        executableTable->selectRow(targetRow);
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    // Get index at viewport position
    QModelIndex index = executableTable->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();

    // Select the clicked item visually
    executableTable->selectRow(row);

    QTableWidgetItem *nameItem = executableTable->item(row, 0);
    if (!nameItem) return;

    QString path = nameItem->data(Qt::UserRole).toString();
    if (!programs.contains(path)) return;

    ProgramInfo *program = programs.value(path, nullptr);
    if (!program) return;

    // CRITICAL: Store the path for use in action slots
    m_rightClickedPath = path;

    // Keep actions enabled
    startRecordingAction->setEnabled(true);
    stopRecordingAction->setEnabled(true);

    QFont normalFont;
    normalFont.setBold(false);
    startRecordingAction->setFont(normalFont);
    stopRecordingAction->setFont(normalFont);

    contextMenu->popup(executableTable->viewport()->mapToGlobal(pos));
}

void MainWindow::removeGame()
{
    // Use the stored right-clicked path
    if (m_rightClickedPath.isEmpty()) return;

    if (!programs.contains(m_rightClickedPath)) return;
    ProgramInfo *program = programs.value(m_rightClickedPath, nullptr);
    if (!program) return;

    QString gameName = program->name();
    int ret = QMessageBox::question(this, "Remove Game",
                                    "Are you sure you want to remove " + gameName + " from the list?",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Remove from programs map
        programs.remove(m_rightClickedPath);
        delete program;

        // Remove from table
        for (int row = 0; row < executableTable->rowCount(); ++row) {
            QTableWidgetItem *item = executableTable->item(row, 0);
            if (item && item->data(Qt::UserRole).toString() == m_rightClickedPath) {
                executableTable->removeRow(row);
                break;
            }
        }

        saveProgramData();
    }
    m_rightClickedPath.clear(); // Clear after use
}

void MainWindow::addExecutableToTable(const QFileInfo &fileInfo)
{
    QString filePath = fileInfo.absoluteFilePath();
    if (programs.contains(filePath)) {
        QMessageBox::information(this, "Already Added", "This executable is already in the list.");
        return;
    }
    ProgramInfo *program = new ProgramInfo(fileInfo.fileName(), filePath);
    programs.insert(filePath, program);
    int row = executableTable->rowCount();
    executableTable->insertRow(row);
    QTableWidgetItem *nameItem = new QTableWidgetItem(program->name());
    QTableWidgetItem *runningItem = new QTableWidgetItem(program->isRunning() ? "Yes" : "No");
    QTableWidgetItem *timeItem = new QTableWidgetItem(program->formattedTimePlayed());
    QTableWidgetItem *recordingItem = new QTableWidgetItem(program->isRecording() ? "Yes" : "No");
    nameItem->setData(Qt::UserRole, filePath);
    if (program->isRunning()) {
        runningItem->setBackground(QBrush(QColor(144, 238, 144)));
    } else {
        runningItem->setBackground(QBrush(QColor(255, 182, 193)));
    }
    if (program->isRecording()) {
        recordingItem->setBackground(QBrush(QColor(255, 165, 0)));
    }
    executableTable->setItem(row, 0, nameItem);
    executableTable->setItem(row, 1, runningItem);
    executableTable->setItem(row, 2, timeItem);
    executableTable->setItem(row, 3, recordingItem);
    executableTable->resizeColumnsToContents();
    saveProgramData();
}

void MainWindow::saveProgramData()
{
    QSettings settings("Stat Tracker", "MineRecordEX");
    settings.beginWriteArray("Programs");
    int index = 0;
    for (auto it = programs.begin(); it != programs.end(); ++it) {
        ProgramInfo *program = it.value();
        settings.setArrayIndex(index);
        settings.setValue("Name", program->name());
        settings.setValue("Path", program->path());
        settings.setValue("TimePlayed", program->timePlayedInSeconds());
        settings.setValue("IsRecording", program->isRecording());
        ++index;
    }
    settings.endArray();
}

void MainWindow::loadProgramData()
{
    QSettings settings("Stat Tracker", "MineRecordEX");
    int size = settings.beginReadArray("Programs");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("Name").toString();
        QString path = settings.value("Path").toString();
        qint64 timePlayed = settings.value("TimePlayed", 0).toLongLong();
        bool isRecording = settings.value("IsRecording", false).toBool();
        if (!name.isEmpty() && !path.isEmpty()) {
            ProgramInfo *program = new ProgramInfo(name, path);
            program->setTimePlayedInSeconds(timePlayed);
            program->setRecording(isRecording);
            programs.insert(path, program);
        }
    }
    settings.endArray();

    // Let updateProgramStatus() handle table population
    // This ensures consistent theming and no auto-selection
    if (executableTable) {
        updateProgramStatus();
    }

    enableDataCollection = settings.value("enableDataCollection", false).toBool();
}

void MainWindow::loadSettings()
{
    QSettings settings("Stat Tracker", "MineRecordEX");
    enableDataCollection = settings.value("enableDataCollection", false).toBool();
}

QString MainWindow::getCurrentThemeStyleSheet() const
{
    QSettings settings("Stat Tracker", "MineRecordEX");
    QString theme = settings.value("theme", "Light").toString();

    if (theme == "Dark") {
        return R"(
            QMainWindow, QDialog, QWidget {
                background-color: #353535;
                color: white;
            }
            QMenuBar {
                background-color: #353535;
                color: green;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 4px 8px;
                color: green;
            }
            QMenuBar::item:selected {
                background-color: #454545;
                color: green;
            }
            QMenuBar::item:pressed {
                background-color: #555555;
                color: green;
            }
            QGroupBox {
                background-color: #2A2A2A;
                border: none;
                margin-top: 1em;
                padding: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 5px;
                background-color: #2A2A2A;
                color: white;
            }
            QListWidget {
                background-color: transparent;
                border: none;
                outline: none;
                padding: 0px;
                margin: 0px;
            }
            QListWidget::item {
                padding: 8px 16px;
                color: white;
                background-color: transparent;
                margin: 0px;
            }
            QListWidget::item:selected {
                background-color: #454545;
                color: white;
            }
            QListWidget::item:hover {
                background-color: #555555;
            }
            QTableWidget {
                background-color: #232323;
                alternate-background-color: #353535;
                color: white;
                gridline-color: #404040;
                selection-background-color: #2A82DA;
                selection-color: black;
                border: 1px solid #404040;
            }
            QHeaderView::section {
                background-color: #353535;
                color: white;
                padding: 4px;
                border: 1px solid #404040;
            }
            QPushButton {
                background-color: #353535;
                color: white;
                border: 1px solid #555555;
                padding: 4px;
                border-radius: 2px;
            }
            QPushButton:hover {
                background-color: #454545;
            }
            QLineEdit, QComboBox, QCheckBox {
                background-color: #232323;
                color: white;
                border: 1px solid #555555;
                padding: 2px;
            }
            /* ComboBox dropdown styling - DARK GREEN HOVER */
            QComboBox QAbstractItemView {
                background-color: #2A2A2A;
                border: 1px solid #404040;
            }
            QComboBox QAbstractItemView::item {
                color: white;
                padding: 4px;
            }
            QComboBox QAbstractItemView::item:hover {
                background-color: #2E8B57;
                color: white;
            }
            QComboBox QAbstractItemView::item:selected {
                background-color: #2E8B57;
                color: white;
            }
            /* Context menu hover effects */
            QMenu {
                background-color: #2A2A2A;
                border: 1px solid #404040;
            }
            QMenu::item {
                padding: 6px 20px 6px 20px;
                color: white;
            }
            QMenu::item:selected {
                background-color: #454545;
                color: white;
            }
            QMenu::item:disabled {
                color: #777777;
            }
        )";
    } else {
        return R"(
            QMainWindow, QDialog, QWidget {
                background-color: #F0F0F0;
                color: black;
            }
            QMenuBar {
                background-color: #F0F0F0;
                color: green;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 4px 8px;
                color: green;
            }
            QMenuBar::item:selected {
                background-color: #E0E0E0;
                color: green;
            }
            QMenuBar::item:pressed {
                background-color: #D0D0D0;
                color: green;
            }
            QGroupBox {
                background-color: #FFFFFF;
                border: none;
                margin-top: 1em;
                padding: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 5px;
                background-color: #FFFFFF;
                color: black;
            }
            QListWidget {
                background-color: transparent;
                border: none;
                outline: none;
                padding: 0px;
                margin: 0px;
            }
            QListWidget::item {
                padding: 8px 16px;
                color: black;
                background-color: transparent;
                margin: 0px;
            }
            QListWidget::item:selected {
                background-color: #F0F0F0;
                color: black;
            }
            QListWidget::item:hover {
                background-color: #F5F5F5;
            }
            QTableWidget {
                background-color: white;
                alternate-background-color: #E6E6E6;
                color: black;
                gridline-color: #CCCCCC;
                selection-background-color: #0078D7;
                selection-color: white;
                border: 1px solid #CCCCCC;
            }
            QHeaderView::section {
                background-color: #F0F0F0;
                color: black;
                padding: 4px;
                border: 1px solid #CCCCCC;
            }
            QPushButton {
                background-color: #F0F0F0;
                color: black;
                border: 1px solid #CCCCCC;
                padding: 4px;
                border-radius: 2px;
            }
            QPushButton:hover {
                background-color: #E0E0E0;
            }
            QLineEdit, QComboBox, QCheckBox {
                background-color: white;
                color: black;
                border: 1px solid #CCCCCC;
                padding: 2px;
            }
            /* ComboBox dropdown styling - DARK GREEN HOVER */
            QComboBox QAbstractItemView {
                background-color: white;
                border: 1px solid #CCCCCC;
            }
            QComboBox QAbstractItemView::item {
                color: black;
                padding: 4px;
            }
            QComboBox QAbstractItemView::item:hover {
                background-color: #2E8B57;
                color: white;
            }
            QComboBox QAbstractItemView::item:selected {
                background-color: #2E8B57;
                color: white;
            }
            /* Context menu hover effects */
            QMenu {
                background-color: white;
                border: 1px solid #CCCCCC;
            }
            QMenu::item {
                padding: 6px 20px 6px 20px;
                color: black;
            }
            QMenu::item:selected {
                background-color: #E0E0E0;
                color: black;
            }
            QMenu::item:disabled {
                color: #999999;
            }
        )";
    }
}

void MainWindow::applySettings()
{
    QString styleSheet = getCurrentThemeStyleSheet();
    qApp->setStyleSheet(styleSheet);

    // Apply to settings dialog if it exists
    if (settingsDialog) {
        settingsDialog->setStyleSheet(styleSheet);
    }

    updateProgramStatus();

    if (executableTable) {
        executableTable->clearSelection();
        executableTable->setCurrentCell(-1, -1);
    }

    QApplication::processEvents();
}

void MainWindow::on_actionRemove_Game_triggered()
{
    if (!executableTable) {
        qDebug() << "Executable table is null";
        return;
    }

    QList<QTableWidgetSelectionRange> selection = executableTable->selectedRanges();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a game from the table first.");
        return;
    }

    int row = selection.first().topRow();
    if (row < 0 || row >= executableTable->rowCount()) {
        qDebug() << "Invalid row selection:" << row;
        return;
    }

    QTableWidgetItem *nameItem = executableTable->item(row, 0);
    if (!nameItem) {
        qDebug() << "Missing name item at row" << row;
        QMessageBox::critical(this, "Error", "Invalid game entry selected.");
        return;
    }

    QString path = nameItem->data(Qt::UserRole).toString();
    if (!programs.contains(path)) {
        qDebug() << "Selected game not found in programs map:" << path;
        return;
    }

    ProgramInfo *program = programs.value(path, nullptr);
    if (!program) {
        qDebug() << "ProgramInfo is null for path:" << path;
        return;
    }

    QString gameName = program->name();
    int ret = QMessageBox::question(this, "Remove Game",
                                    "Are you sure you want to remove " + gameName + " from the list?",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Remove from programs map
        programs.remove(path);
        delete program;

        // Remove from table
        executableTable->removeRow(row);

        saveProgramData();

        // Clear selection after removal
        executableTable->clearSelection();
        executableTable->setCurrentCell(-1, -1);

        QMessageBox::information(this, "Game Removed", gameName + " has been removed from the list.");
    }
}
