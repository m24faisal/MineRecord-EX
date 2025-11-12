#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onCategoryChanged(int index);
    void onOkClicked();
    void onCancelClicked();
    void onApplyClicked();
    void onBrowseRecordingPath();
    void onBrowseExportPath();
    void onThemeChanged(const QString &theme);

private:
    void setupUI();
    void setupGeneralSettings();
    void setupPathSettings();
    void loadSettings();
    void saveSettings();
    void applySettings();

    // Main layout
    QVBoxLayout *mainLayout;
    QHBoxLayout *contentLayout;
    QHBoxLayout *buttonLayout;

    // Widgets
    QListWidget *categoryList;
    QStackedWidget *stackedWidget;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QPushButton *applyButton;

    // General settings widgets
    QWidget *generalSettingsWidget;
    QComboBox *themeComboBox;

    // Path settings widgets
    QWidget *pathSettingsWidget;
    QLineEdit *recordingPathEdit;
    QPushButton *recordingBrowseButton;
    QLineEdit *exportPathEdit;
    QPushButton *exportBrowseButton;

    // Settings values
    QString currentTheme;
    QString recordingPath;
    QString exportPath;
};

#endif // SETTINGSDIALOG_H
