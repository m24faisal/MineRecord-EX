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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    infoDialog(nullptr),  // Initialize info dialog pointer
    settingsDialog(nullptr)  // Initialize settings dialog pointer
{
    ui->setupUi(this);

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

    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Check if this is a mouse press event
    if (event->type() == QEvent::MouseButtonPress) {
        // If the click is outside the table, clear selection
        QWidget *widget = qobject_cast<QWidget*>(watched);
        if (widget && !executableTable->isAncestorOf(widget) && widget != executableTable) {
            executableTable->clearSelection();
        }
    }

    // Pass the event on to the parent class
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUI()
{
    // Create a central widget and layout
    centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Create the executable table
    executableTable = new QTableWidget(this);
    executableTable->setColumnCount(4); // Updated to include recording status
    executableTable->setHorizontalHeaderLabels(QStringList() << "Program Name" << "Running" << "Time Played" << "Recording");
    executableTable->horizontalHeader()->setStretchLastSection(true);
    executableTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    executableTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    executableTable->setSelectionMode(QAbstractItemView::SingleSelection);
    executableTable->setAlternatingRowColors(true);

    // Fix the selection issue
    executableTable->setFocusPolicy(Qt::NoFocus);
    executableTable->setStyleSheet("QTableWidget::item:selected { background: #3399ff; }");

    // Add the table to the layout
    layout->addWidget(executableTable);

    // Set the central widget
    setCentralWidget(centralWidget);

    // Set window properties
    setWindowTitle("Game Manager");
    resize(800, 600);
}

void MainWindow::on_actionAdd_Game_triggered()
{
    // Open a file dialog to select an executable
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Executable",
        QDir::homePath(),
        "Executable Files (*.exe *.bat *.cmd *.app *.sh);;All Files (*)"
        );

    // If the user didn't cancel the dialog
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        addExecutableToTable(fileInfo);
    }
}

void MainWindow::on_actionExit_Application_triggered()
{
    // Close the main window
    this->close();
}

void MainWindow::on_actionGitHub_triggered()
{
    // Open the GitHub repository in the default browser
    QDesktopServices::openUrl(QUrl("https://github.com/m24faisal?tab=repositories"));
}

void MainWindow::on_actionInfo_triggered()
{
    // Create the info dialog if it doesn't exist
    if (!infoDialog) {
        infoDialog = new InfoDialog(this);
    }

    // Show the dialog
    infoDialog->show();
    infoDialog->raise();  // Bring to front
    infoDialog->activateWindow();  // Activate window
}

void MainWindow::on_actionSettings_triggered()
{
    // Create the settings dialog if it doesn't exist
    if (!settingsDialog) {
        settingsDialog = new SettingsDialog(this);
    }

    // Show the dialog
    settingsDialog->show();
    settingsDialog->raise();  // Bring to front
    settingsDialog->activateWindow();  // Activate window
}

void MainWindow::addExecutableToTable(const QFileInfo &fileInfo)
{
    QString filePath = fileInfo.absoluteFilePath();

    // Check if the executable is already in the table
    if (programs.contains(filePath)) {
        QMessageBox::information(this, "Already Added",
                                 "This executable is already in the list.");
        return;
    }

    // Create a new ProgramInfo object
    ProgramInfo *program = new ProgramInfo(fileInfo.fileName(), filePath);

    programs.insert(filePath, program);

    // Add a new row
    int row = executableTable->rowCount();
    executableTable->insertRow(row);

    // Create items for the row
    QTableWidgetItem *nameItem = new QTableWidgetItem(program->name());
    QTableWidgetItem *runningItem = new QTableWidgetItem(program->isRunning() ? "Yes" : "No");
    QTableWidgetItem *timeItem = new QTableWidgetItem(program->formattedTimePlayed());
    QTableWidgetItem *recordingItem = new QTableWidgetItem(program->isRecording() ? "Yes" : "No");

    // Store the path in the name item's user role for later retrieval
    nameItem->setData(Qt::UserRole, filePath);

    // Set the running status color
    if (program->isRunning()) {
        runningItem->setBackground(QBrush(QColor(144, 238, 144))); // Light green
    } else {
        runningItem->setBackground(QBrush(QColor(255, 182, 193))); // Light red
    }

    // Set the recording status color
    if (program->isRecording()) {
        recordingItem->setBackground(QBrush(QColor(255, 165, 0))); // Orange
    }

    // Add items to the table
    executableTable->setItem(row, 0, nameItem);
    executableTable->setItem(row, 1, runningItem);
    executableTable->setItem(row, 2, timeItem);
    executableTable->setItem(row, 3, recordingItem);

    // Resize columns to content
    executableTable->resizeColumnsToContents();

    // Save the updated program data
    saveProgramData();
}

void MainWindow::updateProgramStatus()
{
    // Get the process detector instance
    ProcessDetector &detector = ProcessDetector::instance();

    for (int i = 0; i < executableTable->rowCount(); ++i) {
        QTableWidgetItem *nameItem = executableTable->item(i, 0);
        if (!nameItem) continue;

        QString path = nameItem->data(Qt::UserRole).toString();

        if (programs.contains(path)) {
            ProgramInfo *program = programs[path];

            // Get the current running status using the ProcessDetector
            bool isRunning = detector.isProcessRunning(program->name());

            // Update the running status if it has changed
            if (program->isRunning() != isRunning) {
                program->setRunning(isRunning);

                // Update the table row
                QTableWidgetItem *runningItem = executableTable->item(i, 1);

                if (runningItem) {
                    runningItem->setText(isRunning ? "Yes" : "No");

                    // Set the running status color
                    if (isRunning) {
                        runningItem->setBackground(QBrush(QColor(144, 238, 144))); // Light green
                    } else {
                        runningItem->setBackground(QBrush(QColor(255, 182, 193))); // Light red
                    }
                }
            }

            // Always update the time played
            QTableWidgetItem *timeItem = executableTable->item(i, 2);
            if (timeItem) {
                timeItem->setText(program->formattedTimePlayed());
            }

            // Update recording status
            QTableWidgetItem *recordingItem = executableTable->item(i, 3);
            if (recordingItem) {
                recordingItem->setText(program->isRecording() ? "Yes" : "No");

                // Set the recording status color
                if (program->isRecording()) {
                    recordingItem->setBackground(QBrush(QColor(255, 165, 0))); // Orange
                } else {
                    recordingItem->setBackground(QBrush()); // Default
                }
            }
        }
    }
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
            // Create a new ProgramInfo object
            ProgramInfo *program = new ProgramInfo(name, path);
            program->setTimePlayedInSeconds(timePlayed);
            program->setRecording(isRecording);

            programs.insert(path, program);

            // Add a new row
            int row = executableTable->rowCount();
            executableTable->insertRow(row);

            // Create items for the row
            QTableWidgetItem *nameItem = new QTableWidgetItem(name);
            QTableWidgetItem *runningItem = new QTableWidgetItem(program->isRunning() ? "Yes" : "No");
            QTableWidgetItem *timeItem = new QTableWidgetItem(program->formattedTimePlayed());
            QTableWidgetItem *recordingItem = new QTableWidgetItem(program->isRecording() ? "Yes" : "No");

            // Store the path in the name item's user role for later retrieval
            nameItem->setData(Qt::UserRole, path);

            // Set the running status color
            if (program->isRunning()) {
                runningItem->setBackground(QBrush(QColor(144, 238, 144))); // Light green
            } else {
                runningItem->setBackground(QBrush(QColor(255, 182, 193))); // Light red
            }

            // Set the recording status color
            if (program->isRecording()) {
                recordingItem->setBackground(QBrush(QColor(255, 165, 0))); // Orange
            }

            // Add items to the table
            executableTable->setItem(row, 0, nameItem);
            executableTable->setItem(row, 1, runningItem);
            executableTable->setItem(row, 2, timeItem);
            executableTable->setItem(row, 3, recordingItem);
        }
    }

    settings.endArray();

    // Resize columns to content after loading
    executableTable->resizeColumnsToContents();
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    // Get the item at the position
    QTableWidgetItem *item = executableTable->itemAt(pos);

    if (item) {
        // Select the row
        executableTable->selectRow(item->row());

        // Get the program path
        QString path = executableTable->item(item->row(), 0)->data(Qt::UserRole).toString();
        ProgramInfo *program = programs.value(path, nullptr);

        if (program) {
            // Enable/disable actions based on program state
            removeAction->setEnabled(true);

            // Enable/disable recording actions based on recording state
            startRecordingAction->setEnabled(!program->isRecording());
            stopRecordingAction->setEnabled(program->isRecording());

            // Show the context menu
            contextMenu->exec(executableTable->viewport()->mapToGlobal(pos));
        }
    }
}

void MainWindow::removeGame()
{
    // Get the selected row
    int currentRow = executableTable->currentRow();

    if (currentRow >= 0) {
        // Get the program path
        QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();

        // Ask for confirmation
        int ret = QMessageBox::question(this, "Remove Game",
                                        "Are you sure you want to remove this game from the list?",
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            // Remove from programs map
            if (programs.contains(path)) {
                delete programs.take(path);
            }

            // Remove from table
            executableTable->removeRow(currentRow);

            // Save the updated program data
            saveProgramData();
        }
    }
}

void MainWindow::startRecording()
{
    // Get the selected row
    int currentRow = executableTable->currentRow();

    if (currentRow >= 0) {
        // Get the program path
        QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();
        ProgramInfo *program = programs.value(path, nullptr);

        if (program) {
            // Set recording state
            program->setRecording(true);

            // Update the recording status in the table
            QTableWidgetItem *recordingItem = executableTable->item(currentRow, 3);
            if (recordingItem) {
                recordingItem->setText("Yes");
                recordingItem->setBackground(QBrush(QColor(255, 165, 0))); // Orange
            }

            QMessageBox::information(this, "Start Recording",
                                     "Recording started for " + program->name());

            // Save the updated program data
            saveProgramData();
        }
    }
}

void MainWindow::stopRecording()
{
    // Get the selected row
    int currentRow = executableTable->currentRow();

    if (currentRow >= 0) {
        // Get the program path
        QString path = executableTable->item(currentRow, 0)->data(Qt::UserRole).toString();
        ProgramInfo *program = programs.value(path, nullptr);

        if (program) {
            // Set recording state
            program->setRecording(false);

            // Update the recording status in the table
            QTableWidgetItem *recordingItem = executableTable->item(currentRow, 3);
            if (recordingItem) {
                recordingItem->setText("No");
                recordingItem->setBackground(QBrush()); // Default
            }

            QMessageBox::information(this, "Stop Recording",
                                     "Recording stopped for " + program->name());

            // Save the updated program data
            saveProgramData();
        }
    }
}

void MainWindow::applySettings()
{
    // Load settings from QSettings
    QSettings settings("YourCompany", "GameManager");

    // Apply theme
    QString theme = settings.value("theme", "Default").toString();
    if (theme == "Dark") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
        // Additional dark theme styling could be added here
    } else if (theme == "Light") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
        // Additional light theme styling could be added here
    } else {
        // Default theme
        QApplication::setStyle(QStyleFactory::create("windowsvista"));
    }
}
