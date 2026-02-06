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
#include <QCloseEvent>
#include "ui_mainwindow.h"

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
    void applySettings();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

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

private slots:
    void cleanupBeforeQuit();
    void handleDataCollectionError(QProcess::ProcessError error);
    void onAppFocusChanged(QWidget *old, QWidget *now);

private:
    // UI Setup and Management
    void setupUI();
    void addExecutableToTable(const QFileInfo &fileInfo);
    void saveProgramData();
    void loadProgramData();
    void loadSettings();
    void recreateExecutableTable();
    QString m_rightClickedPath;
    QString getCurrentThemeStyleSheet() const;
    bool startRecordingProcess(const QString &executablePath, const QString &recordingDir, const QString &gameName);
    bool stopRecordingProcess(const QString &gameName);

    // Python Backend Management
    void initializePythonBackend();
    void shutdownPythonBackend();
    QString startPythonRecording(const QString &gameName, const QString &gamePath, const QString &recordingPath);
    QString stopPythonRecording(const QString &recordingId);

    // --- Member Variables ---

    Ui::MainWindow *ui;
    PythonBackendWrapper *m_pythonWrapper;
    QTableWidget *executableTable;
    QWidget *centralWidget;
    QTimer *updateTimer;
    QMap<QString, ProgramInfo*> programs;
    QMenu *contextMenu;
    QAction *removeAction;
    QAction *startRecordingAction;
    QAction *stopRecordingAction;
    QDialog *infoDialog;
    SettingsDialog *settingsDialog;
    QString activeRecordingId;
    bool enableDataCollection;

    QString m_lastSelectedExecutablePath;
    bool m_isRecordingActionInProgress = false;

    // Store active processes for cleanup
    QMap<QString, QProcess*> m_activeProcesses;
};

#endif // MAINWINDOW_H
