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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    // --- CORRECTED: Initializer list order now matches header declaration order ---
    m_pythonWrapper(nullptr),
    ui(new Ui::MainWindow),
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
    dataCollectionProcess(nullptr),
    enableDataCollection(false)
{
    ui->setupUi(this);

    loadSettings();
    qDebug() << "SETTINGS LOADED. enableDataCollection is:" << enableDataCollection;

    applySettings();
    setupUI();

    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateProgramStatus()));
    updateTimer->start(1000);

    ProcessDetector::instance().setUpdateInterval(500);
    connect(&ProcessDetector::instance(), SIGNAL(processListUpdated()),
            this, SLOT(updateProgramStatus()));

    qApp->installEventFilter(this);

    contextMenu = new QMenu(this);
    removeAction = new QAction("Remove Game", this);
    startRecordingAction = new QAction("Start Recording", this);
    stopRecordingAction = new QAction("Stop Recording", this);

    contextMenu->addAction(removeAction);
    contextMenu->addAction(startRecordingAction);
    contextMenu->addAction(stopRecordingAction);

    executableTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(executableTable, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));

    loadProgramData();

    // --- Python Integration ---
    m_pythonWrapper = new PythonBackendWrapper();
    m_pythonWrapper->initialize();

    // --- Start the global data collection service IF AND ONLY IF the setting is enabled ---
    if (enableDataCollection) {
        qDebug() << "Data collection is ENABLED. Attempting to start global Python service NOW.";
        m_pythonWrapper->startDataService(); // Correct function name
    } else {
        qDebug() << "Data collection is DISABLED. Service will not be started.";
    }
}

MainWindow::~MainWindow()
{
    qDebug() << "Stopping global Python data collection service...";
    if (m_pythonWrapper) {
        m_pythonWrapper->stopDataService();
        delete m_pythonWrapper;
        m_pythonWrapper = nullptr;
    }

    saveProgramData();
    qDeleteAll(programs);
    programs.clear();

    if (infoDialog) {
        delete infoDialog;
    }
    if (settingsDialog) {
        delete settingsDialog;
    }
    delete ui;
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
    // Handled in destructor
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

// --- Recording Slots ---
void MainWindow::startRecording()
{
    int currentRow = executableTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "No Game Selected", "Please select a game from the list to start recording.");
        return;
    }

    QTableWidgetItem *pathItem = executableTable->item(currentRow, 0);
    if (!pathItem) return;

    QString path = pathItem->data(Qt::UserRole).toString();
    ProgramInfo *program = programs.value(path, nullptr);
    if (!program) return;

    QSettings settings("YourCompany", "GameManager");
    QString recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();

    QString result = startPythonRecording(program->name(), program->path(), recordingPath);

    if (result.startsWith("Error:")) {
        QMessageBox::warning(this, "Recording Failed", result);
        return;
    }

    program->setRecording(true);
    program->setRecordingId(result); // Save the ID we got from Python
    activeRecordingId = result;

    QTableWidgetItem *recordingItem = executableTable->item(currentRow, 3);
    if (recordingItem) {
        recordingItem->setText("Yes");
        recordingItem->setBackground(QBrush(QColor(255, 165, 0)));
    }
    QMessageBox::information(this, "Recording Started", "Recording started for " + program->name());
    saveProgramData();
}

void MainWindow::stopRecording()
{
    int currentRow = executableTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "No Game Selected", "Please select a game from the list to stop recording.");
        return;
    }

    QTableWidgetItem *pathItem = executableTable->item(currentRow, 0);
    if (!pathItem) return;

    QString path = pathItem->data(Qt::UserRole).toString();
    ProgramInfo *program = programs.value(path, nullptr);
    if (!program) return;

    QString recordingId = program->recordingId();

    QString result = stopPythonRecording(recordingId);

    if (result.startsWith("Error:")) {
        QMessageBox::warning(this, "Recording Failed", result);
        return;
    }

    // Update the program's state
    program->setRecording(false);
    QTableWidgetItem *recordingItem = executableTable->item(currentRow, 3);
    if (recordingItem) {
        recordingItem->setText("No");
        recordingItem->setBackground(QBrush());
    }

    // Clear the global active ID
    activeRecordingId.clear();

    QMessageBox::information(this, "Recording Stopped", "Recording stopped for " + program->name());
    saveProgramData();
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
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && !executableTable->isAncestorOf(widget)) {
            executableTable->clearSelection();
        }
    }
    return QMainWindow::eventFilter(watched, event);
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
    executableTable->setFocusPolicy(Qt::NoFocus);
    executableTable->setStyleSheet("QTableWidget::item:selected { background: #3399ff; }");
    layout->addWidget(executableTable);
    setCentralWidget(centralWidget);
    setWindowTitle("Game Manager");
    resize(800, 600);
}

void MainWindow::on_actionAdd_Game_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select Executable", QDir::homePath(), "Executable Files (*.exe *.bat *.cmd *.app *.sh);;All Files (*)");
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        addExecutableToTable(fileInfo);
    }
}

void MainWindow::on_actionExit_Application_triggered()
{ this->close(); }

void MainWindow::on_actionGitHub_triggered()
{ QDesktopServices::openUrl(QUrl("https://github.com/m24faisal?tab=repositories ")); }

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
        settingsDialog = new SettingsDialog(this, [this](const QString &playerName, const QString &exportPath) {
            if (!m_pythonWrapper) {
                return QString("Error: Python wrapper is not initialized.");
            }
            std::string result = m_pythonWrapper->exportPlayerData(playerName.toStdString(), exportPath.toStdString());
            return QString::fromStdString(result);
        });
    }
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void MainWindow::updateProgramStatus()
{
    ProcessDetector &detector = ProcessDetector::instance();
    for (int i = 0; i < executableTable->rowCount(); ++i) {
        QTableWidgetItem *nameItem = executableTable->item(i, 0);
        if (!nameItem) continue;

        QString path = nameItem->data(Qt::UserRole).toString();
        if (programs.contains(path)) {
            ProgramInfo *program = programs.value(path, nullptr);
            if (program) {
                bool isRunning = detector.isProcessRunning(program->name());
                if (program->isRunning() != isRunning) {
                    program->setRunning(isRunning);
                    QTableWidgetItem *runningItem = executableTable->item(i, 1);
                    if (runningItem) {
                        runningItem->setText(isRunning ? "Yes" : "No");
                        if (isRunning) {
                            runningItem->setBackground(QBrush(QColor(144, 238, 144)));
                        } else {
                            runningItem->setBackground(QBrush(QColor(255, 182, 193)));
                        }
                    }
                }

                QTableWidgetItem *timeItem = executableTable->item(i, 2);
                if (timeItem) {
                    timeItem->setText(program->formattedTimePlayed());
                }

                QTableWidgetItem *recordingItem = executableTable->item(i, 3);
                if (recordingItem) {
                    recordingItem->setText(program->isRecording() ? "Yes" : "No");
                    if (program->isRecording()) {
                        recordingItem->setBackground(QBrush(QColor(255, 165, 0)));
                    } else {
                        recordingItem->setBackground(QBrush());
                    }
                }
            }
        }
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = executableTable->itemAt(pos);
    if (item) {
        executableTable->selectRow(item->row());
        QString path = item->data(Qt::UserRole).toString();
        ProgramInfo *program = programs.value(path, nullptr);
        if (program) {
            removeAction->setEnabled(true);
            startRecordingAction->setEnabled(!program->isRecording());
            stopRecordingAction->setEnabled(program->isRecording());
            contextMenu->exec(executableTable->viewport()->mapToGlobal(pos));
        }
    }
}

void MainWindow::removeGame()
{
    int currentRow = executableTable->currentRow();
    if (currentRow >= 0) {
        QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();
        int ret = QMessageBox::question(this, "Remove Game", "Are you sure you want to remove this game from the list?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            if (programs.contains(path)) {
                delete programs.take(path);
            }
            executableTable->removeRow(currentRow);
            saveProgramData();
        }
    }
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
    QSettings settings("YourCompany", "GameManager");
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
    QSettings settings("YourCompany", "GameManager");
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
            int row = executableTable->rowCount();
            executableTable->insertRow(row);
            QTableWidgetItem *nameItem = new QTableWidgetItem(name);
            QTableWidgetItem *runningItem = new QTableWidgetItem(program->isRunning() ? "Yes" : "No");
            QTableWidgetItem *timeItem = new QTableWidgetItem(program->formattedTimePlayed());
            QTableWidgetItem *recordingItem = new QTableWidgetItem(program->isRecording() ? "Yes" : "No");
            nameItem->setData(Qt::UserRole, path);
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
        }
    }
    settings.endArray();
    executableTable->resizeColumnsToContents();
    enableDataCollection = settings.value("enableDataCollection", false).toBool();
}

void MainWindow::loadSettings()
{
    qDebug() << "loadSettings() called.";
    QSettings settings("YourCompany", "GameManager");
    bool dataCollectionSetting = settings.value("enableDataCollection", false).toBool();
    qDebug() << "Value from QSettings is:" << dataCollectionSetting;
    enableDataCollection = dataCollectionSetting;
    qDebug() << "Member variable enableDataCollection is now set to:" << enableDataCollection;
}

void MainWindow::applySettings()
{
    qDebug() << "applySettings() called.";
    QSettings settings("YourCompany", "GameManager");
    QString theme = settings.value("theme", "Default").toString();
    if (theme == "Dark") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
    } else if (theme == "Light") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
    } else {
        QApplication::setStyle(QStyleFactory::create("windowsvista"));
    }
    QString recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();
    QDir dir;
    if (!dir.exists(recordingPath)) {
        dir.mkpath(recordingPath);
    }
}
