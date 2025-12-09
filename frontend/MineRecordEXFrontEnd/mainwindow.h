#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// --- CRITICAL FIX: Include pybind11 FIRST ---
// This prevents Qt's macros (like 'slots') from interfering with pybind11's headers.
#include <pybind11/embed.h>

// --- CRITICAL FIX: Define the pybind11 namespace alias at global scope ---
// This makes the 'py::' alias available to the rest of this file.
namespace py = pybind11;


// --- Now, include Qt headers ---
#include <QMainWindow>
#include <QTableWidget>
#include <QFileInfo>
#include <QTimer>
#include <QMap>
#include <QMenu>
#include <QEvent>
#include <QMessageBox>

// --- Forward declare the Ui namespace ---
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// --- Forward declare other custom classes ---
class ProgramInfo;
class InfoDialog;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void on_actionAdd_Game_triggered();
    void on_actionExit_Application_triggered();
    void on_actionGitHub_triggered();
    void on_actionInfo_triggered();
    void on_actionSettings_triggered();
    void updateProgramStatus();
    void showContextMenu(const QPoint &pos);
    void removeGame();
    void startRecording();
    void stopRecording();

private:
    void setupUI();
    void addExecutableToTable(const QFileInfo &fileInfo);
    void saveProgramData();
    void loadProgramData();
    void loadSettings();
    void applySettings();
    void shutdownPythonBackend();

    // Python communication methods
    void initializePythonBackend();
    QString startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath);
    QString stopPythonRecording(const QString &recordingId);

    // The pybind11 interpreter guard is NOT a member variable.
    // It is initialized once in main.cpp.

    // pybind11 specific members
    py::module backend_module;

    // UI member
    Ui::MainWindow *ui;

    // Other UI and state members
    QTableWidget *executableTable;
    QWidget *centralWidget;
    QTimer *updateTimer;
    QMap<QString, ProgramInfo*> programs;
    QMenu *contextMenu;
    QAction *removeAction;
    QAction *startRecordingAction;
    QAction *stopRecordingAction;
    InfoDialog *infoDialog;
    SettingsDialog *settingsDialog;
    QString activeRecordingId;

    // Settings
    bool enableDataCollection;
};

#endif // MAINWINDOW_H
