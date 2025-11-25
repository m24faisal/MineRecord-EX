#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "programinfo.h"
#include "processdetector.h"
#include "infodialog.h"
#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QSettings>
#include <QDateTime>
#include <QEvent>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QStyleFactory>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    infoDialog(nullptr),
    settingsDialog(nullptr)
{
    ui->setupUi(this);

    // Load settings before setting up UI
    loadSettings();

    // Apply settings before setting up UI
    applySettings();

    // Set up the main window UI
    setupUI();

    // Set up a timer to update program status
    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateProgramStatus()));
    updateTimer->start(1000); // Update every 1 second (faster)

    // Configure the process detector for faster updates
    ProcessDetector::instance().setUpdateInterval(500); // Update every 500ms

    // Connect to the process detector's update signal
    connect(&ProcessDetector::instance(), SIGNAL(processListUpdated()),
            this, SLOT(updateProgramStatus()));

    // Install event filter on the application to catch click events
    qApp->installEventFilter(this);

    // Create context menu
    contextMenu = new QMenu(this);
    removeAction = new QAction("Remove Game", this);
    startRecordingAction = new QAction("Start Recording", this);
    stopRecordingAction = new QAction("Stop Recording", this);

    contextMenu->addAction(removeAction);
    contextMenu->addAction(startRecordingAction);
    contextMenu->addAction(stopRecordingAction);

    // Connect context menu actions to slots
    connect(removeAction, &QAction::triggered, this, &MainWindow::removeGame);
    connect(startRecordingAction, &QAction::triggered, this, &MainWindow::startRecording);
    connect(stopRecordingAction, &QAction::triggered, this, &MainWindow::stopRecording);

    // Enable context menu on the table
    executableTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(executableTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);

    // Load saved program data
    loadProgramData();

    // --- Python Integration ---
    initializePythonBackend();
}

MainWindow::~MainWindow()
{
    // Save program data before closing
    saveProgramData();

    // Clean up
    qDeleteAll(programs);
    programs.clear();

    // Delete dialogs if they exist
    if (infoDialog) {
        delete infoDialog;
    }

    if (settingsDialog) {
        delete settingsDialog;
    }

    // Python interpreter is finalized automatically by the `py::scoped_interpreter` guard
    delete ui;
}

// --- Python Backend Methods ---
void MainWindow::initializePythonBackend()
{
    try {
        py::exec("import sys; sys.path.append('./backend')");
        backend_module = py::module::import("backend_controller");
        qDebug() << "Python backend initialized successfully.";
    } catch (const py::error_already_set &e) {
        QString errorMsg = "Python initialization failed: ";
        errorMsg += e.what();
        QMessageBox::critical(this, "Python Error", errorMsg);
    }
}

QString MainWindow::startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath)
{
    if (!backend_module) return "Error: Python backend is not initialized.";
    try {
        py::object result = backend_module.attr("start_recording")(
            gameName.toStdString(),
            gamePath.toStdString(),
            recordingPath.toStdString(),
            enableDataCollection  // Pass the data collection setting
            );
        return QString::fromStdString(result.cast<std::string>());
    } catch (const py::error_already_set &e) {
        return QString("Python Error: %1").arg(e.what());
    }
}

QString MainWindow::stopPythonRecording(const QString &recordingId)
{
    if (!backend_module) return "Error: Python backend is not initialized.";
    try {
        py::object result = backend_module.attr("stop_recording")(recordingId.toStdString());
        return QString::fromStdString(result.cast<std::string>());
    } catch (const py::error_already_set &e) {
        return QString("Python Error: %1").arg(e.what());
    }
}

// --- Modified Recording Slots ---
void MainWindow::startRecording()
{
    int currentRow = executableTable->currentRow(); if (currentRow < 0) return;
    QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();
    ProgramInfo *program = programs.value(path, nullptr); if (!program) return;

    QSettings settings("YourCompany", "GameManager");
    QString recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();

    QString result = startPythonRecording(program->name(), program->path(), recordingPath);

    if (result.startsWith("Error:")) {
        QMessageBox::warning(this, "Recording Failed", result);
        return;
    }

    program->setRecording(true);
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
    int currentRow = executableTable->currentRow(); if (currentRow < 0) return;
    QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();
    ProgramInfo *program = programs.value(path, nullptr); if (!program) return;

    QString result = stopPythonRecording(activeRecordingId);
    if (result.startsWith("Error:")) {
        QMessageBox::warning(this, "Recording Failed", result);
        return;
    }

    program->setRecording(false);
    activeRecordingId.clear();
    QTableWidgetItem *recordingItem = executableTable->item(currentRow, 3);
    if (recordingItem) {
        recordingItem->setText("No");
        recordingItem->setBackground(QBrush());
    }
    QMessageBox::information(this, "Recording Stopped", "Recording stopped for " + program->name());
    saveProgramData();
}


// --- All other existing methods ---
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && !executableTable->isAncestorOf(widget) && widget != executableTable) {
            executableTable->clearSelection();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUI() {
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

void MainWindow::on_actionAdd_Game_triggered() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Executable", QDir::homePath(), "Executable Files (*.exe *.bat *.cmd *.app *.sh);;All Files (*)");
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        addExecutableToTable(fileInfo);
    }
}

void MainWindow::on_actionExit_Application_triggered() { this->close(); }
void MainWindow::on_actionGitHub_triggered() { QDesktopServices::openUrl(QUrl("https://github.com/m24faisal?tab=repositories")); }
void MainWindow::on_actionInfo_triggered() {
    if (!infoDialog) infoDialog = new InfoDialog(this);
    infoDialog->show();
    infoDialog->raise();
    infoDialog->activateWindow();
}
void MainWindow::on_actionSettings_triggered() {
    if (!settingsDialog) settingsDialog = new SettingsDialog(this);
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void MainWindow::addExecutableToTable(const QFileInfo &fileInfo) {
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

void MainWindow::updateProgramStatus() {
    ProcessDetector &detector = ProcessDetector::instance();
    for (int i = 0; i < executableTable->rowCount(); ++i) {
        QTableWidgetItem *nameItem = executableTable->item(i, 0);
        if (!nameItem) continue;
        QString path = nameItem->data(Qt::UserRole).toString();
        if (programs.contains(path)) {
            ProgramInfo *program = programs[path];
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

void MainWindow::saveProgramData() {
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
void MainWindow::loadSettings()
{
    QSettings settings("YourCompany", "GameManager");

    // Load theme
    // currentTheme = settings.value("theme", "Default").toString(); // applySettings() handles this

    // Load paths
    recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();
    exportPath = settings.value("exportPath", QDir::homePath() + "/GameExports").toString();

    // Load data collection setting
    enableDataCollection = settings.value("enableDataCollection", true).toBool();
}

void MainWindow::loadProgramData() {
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

    // Load data collection setting
    enableDataCollection = settings.value("enableDataCollection", false).toBool();
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QTableWidgetItem *item = executableTable->itemAt(pos);
    if (item) {
        executableTable->selectRow(item->row());
        QString path = executableTable->item(item->row(), 0)->data(Qt::UserRole).toString();
        ProgramInfo *program = programs.value(path, nullptr);
        if (program) {
            removeAction->setEnabled(true);
            startRecordingAction->setEnabled(!program->isRecording());
            stopRecordingAction->setEnabled(program->isRecording());
            contextMenu->exec(executableTable->viewport()->mapToGlobal(pos));
        }
    }
}

void MainWindow::removeGame() {
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

void MainWindow::applySettings() {
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

    // Load data collection setting
    enableDataCollection = settings.value("enableDataCollection", false).toBool();
}
