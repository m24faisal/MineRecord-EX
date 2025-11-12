#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    void on_actionExit_Application_triggered();  // Slot for exit action
    void on_actionGitHub_triggered();  // Slot for GitHub action
    void on_actionInfo_triggered();  // Slot for Info action
    void on_actionSettings_triggered();  // Slot for Settings action
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
    void applySettings();

    Ui::MainWindow *ui;
    QTableWidget *executableTable;
    QWidget *centralWidget;
    QTimer *updateTimer;
    QMap<QString, ProgramInfo*> programs;
    QMenu *contextMenu;
    QAction *removeAction;
    QAction *startRecordingAction;
    QAction *stopRecordingAction;
    InfoDialog *infoDialog;  // Info dialog instance
    SettingsDialog *settingsDialog;  // Settings dialog instance
};
#endif // MAINWINDOW_H
