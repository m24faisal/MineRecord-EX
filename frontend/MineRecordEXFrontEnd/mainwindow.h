#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// --- Include Qt headers ONLY ---
#include <QMainWindow>
#include <QTableWidget>
#include <QFileInfo>
#include <QTimer>
#include <QMap>
#include <QMenu>
#include <QEvent>
#include <QMessageBox>
#include <QProcess>

// --- Forward declare the Qt-generated UI namespace ---
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// --- Forward declare custom classes ---
class ProgramInfo;
class InfoDialog;
class SettingsDialog;
class PythonBackendWrapper;

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
    void on_actionStart_Recording_triggered();
    void on_actionStop_Recording_triggered();
    void updateProgramStatus();
    void showContextMenu(const QPoint &pos);
    void removeGame();
    void startRecording();
    void stopRecording();

private slots:
    void handleDataCollectionError(QProcess::ProcessError error);

private:
    // UI Setup and Management
    void setupUI();
    void addExecutableToTable(const QFileInfo &fileInfo);
    void saveProgramData();
    void loadProgramData();
    void loadSettings();
    void applySettings();

    // Python Backend Management (Methods are now simpler)
    void initializePythonBackend();
    void shutdownPythonBackend();
    QString startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath);
    QString stopPythonRecording(const QString &recordingId);

    // --- Member Variables ---
    // NOTE: The order here is important. The constructor's initializer list must match this order.

    // Python backend wrapper - NO pybind11 types here!
    PythonBackendWrapper* m_pythonWrapper;

    // UI member (from Qt Designer)
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
    QProcess *dataCollectionProcess; // For the standalone server

    // Settings
    bool enableDataCollection;
};

#endif // MAINWINDOW_H
