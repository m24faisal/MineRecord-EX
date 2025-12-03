#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// --- STEP 1: Include pybind11 FIRST ---
#include <pybind11/embed.h>
namespace py = pybind11;

// --- STEP 2: Now include Qt headers ---
#include <QMainWindow>
#include <QTableWidget>
#include <QFileInfo>
#include <QTimer>
#include <QMap>
#include <QMenu>

// Forward declarations
class ProgramInfo;
class InfoDialog;
class SettingsDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
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
    void on_actionStart_Recording_triggered();
    void on_actionStop_Recording_triggered();

private:
    void setupUI();
    void addExecutableToTable(const QFileInfo &fileInfo);
    void saveProgramData();
    void loadProgramData();
    void loadSettings();
    void applySettings();

    // Python communication methods
    void initializePythonBackend();
    QString startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath);
    QString stopPythonRecording(const QString &recordingId);
    void shutdownPythonBackend();

    // pybind11 specific
    py::scoped_interpreter guard{}; // RAII guard for the interpreter
    py::module backend_module;      // Holds the imported Python module

    Ui::MainWindow *ui;
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
    bool enableDataCollection;  // New setting to track data collection preference
    QString recordingPath;
    QString exportPath;
};

#endif // MAINWINDOW_H
