// settingsdialog.cpp
#include "settingsdialog.h"
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCheckBox>
#include <QPushButton>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>

SettingsDialog::SettingsDialog(QWidget *parent, std::function<QString(const QString&, const QString&)> exportFunc)
    : QDialog(parent),
    m_exportDataFunction(exportFunc)
{
    setupUI();
    loadSettings();
    setWindowTitle("Settings");
    setFixedSize(600, 450);
    setModal(true);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    contentLayout = new QHBoxLayout();
    buttonLayout = new QHBoxLayout();

    // Create category list
    categoryList = new QListWidget(this);
    categoryList->addItem("General");
    categoryList->addItem("Paths");
    categoryList->addItem("DB Settings");
    categoryList->setCurrentRow(0);
    categoryList->setMaximumWidth(150);

    // Create stacked widget for settings pages
    stackedWidget = new QStackedWidget(this);

    // Setup settings pages
    setupGeneralSettings();
    setupPathSettings();
    setupDbSettings();

    // Add pages to stacked widget
    stackedWidget->addWidget(generalSettingsWidget);
    stackedWidget->addWidget(pathSettingsWidget);
    stackedWidget->addWidget(dbSettingsWidget);

    // Create buttons
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    applyButton = new QPushButton("Apply", this);

    // Add buttons to layout
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);

    // Add widgets to content layout
    contentLayout->addWidget(categoryList);
    contentLayout->addWidget(stackedWidget);

    // Add all to main layout
    mainLayout->addLayout(contentLayout);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(categoryList, SIGNAL(currentRowChanged(int)), this, SLOT(onCategoryChanged(int)));
    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
    connect(recordingBrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseRecordingPath()));
    connect(exportBrowseButton, SIGNAL(clicked()), this, SLOT(onBrowseExportPath()));
    connect(themeComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(onThemeChanged(QString)));
    connect(exportDataButton, SIGNAL(clicked()), this, SLOT(onExportDataClicked()));
}

void SettingsDialog::setupGeneralSettings()
{
    generalSettingsWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(generalSettingsWidget);

    // Theme settings group
    QGroupBox *themeGroup = new QGroupBox("Theme", this);
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroup);

    // Theme selection
    QHBoxLayout *themeSelectLayout = new QHBoxLayout();
    QLabel *themeLabel = new QLabel("Application Theme:", this);
    themeComboBox = new QComboBox(this);

    // Add available themes
    themeComboBox->addItem("Default");
    themeComboBox->addItem("Dark");
    themeComboBox->addItem("Light");

    themeSelectLayout->addWidget(themeLabel);
    themeSelectLayout->addWidget(themeComboBox);
    themeLayout->addLayout(themeSelectLayout);

    layout->addWidget(themeGroup);
    layout->addStretch();
}

void SettingsDialog::setupPathSettings()
{
    pathSettingsWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pathSettingsWidget);

    // Recording path settings
    QGroupBox *recordingGroup = new QGroupBox("Recording Path", this);
    QVBoxLayout *recordingLayout = new QVBoxLayout(recordingGroup);

    QHBoxLayout *recordingPathLayout = new QHBoxLayout();
    QLabel *recordingLabel = new QLabel("Recording Save Path:", this);
    recordingPathEdit = new QLineEdit(this);
    recordingBrowseButton = new QPushButton("Browse...", this);

    recordingPathLayout->addWidget(recordingPathEdit);
    recordingPathLayout->addWidget(recordingBrowseButton);

    recordingLayout->addWidget(recordingLabel);
    recordingLayout->addLayout(recordingPathLayout);

    // Export path settings
    QGroupBox *exportGroup = new QGroupBox("Export Path", this);
    QVBoxLayout *exportLayout = new QVBoxLayout(exportGroup);

    QHBoxLayout *exportPathLayout = new QHBoxLayout();
    QLabel *exportLabel = new QLabel("Export Data Path:", this);
    exportPathEdit = new QLineEdit(this);
    exportBrowseButton = new QPushButton("Browse...", this);

    exportPathLayout->addWidget(exportPathEdit);
    exportPathLayout->addWidget(exportBrowseButton);

    exportLayout->addWidget(exportLabel);
    exportLayout->addLayout(exportPathLayout);

    // Data collection settings
    QGroupBox *dataCollectionGroup = new QGroupBox("Data Collection", this);
    QVBoxLayout *dataCollectionLayout = new QVBoxLayout(dataCollectionGroup);

    enableDataCollectionCheckBox = new QCheckBox("Enable data collection", this);
    enableDataCollectionCheckBox->setToolTip("When checked, game data will be collected through receive.py during recording");

    dataCollectionLayout->addWidget(enableDataCollectionCheckBox);

    // Create Data Export button
    exportDataButton = new QPushButton("Export Data", this);
    exportDataButton->setObjectName("exportDataButton");
    exportDataButton->setToolTip("Exports all collected player data to a CSV file.");

    layout->addWidget(recordingGroup);
    layout->addWidget(exportGroup);
    layout->addWidget(dataCollectionGroup);
    layout->addWidget(exportDataButton);
    layout->addStretch();
}

void SettingsDialog::setupDbSettings()
{
    dbSettingsWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(dbSettingsWidget);

    QGroupBox *dbGroup = new QGroupBox("PostgreSQL Database Credentials", this);
    QFormLayout *formLayout = new QFormLayout(dbGroup);

    dbUsernameEdit = new QLineEdit(this);
    dbPasswordEdit = new QLineEdit(this);
    dbPasswordEdit->setEchoMode(QLineEdit::Password); // Mask password

    formLayout->addRow("Username:", dbUsernameEdit);
    formLayout->addRow("Password:", dbPasswordEdit);

    layout->addWidget(dbGroup);
    layout->addStretch();
}

void SettingsDialog::loadSettings()
{
    QSettings settings("YourCompany", "GameManager");

    // Load theme
    currentTheme = settings.value("theme", "Default").toString();
    int themeIndex = themeComboBox->findText(currentTheme);
    if (themeIndex >= 0) {
        themeComboBox->setCurrentIndex(themeIndex);
    }

    // Load paths
    recordingPath = settings.value("recordingPath", QDir::homePath() + "/GameRecordings").toString();
    recordingPathEdit->setText(recordingPath);

    exportPath = settings.value("exportPath", QDir::homePath() + "/GameExports").toString();
    exportPathEdit->setText(exportPath);

    // Load data collection setting
    enableDataCollection = settings.value("enableDataCollection", false).toBool();
    enableDataCollectionCheckBox->setChecked(enableDataCollection);

    // --- NEW: Load DB credentials ---
    QString dbUsername = settings.value("dbUsername", "postgres").toString();
    QString dbPassword = settings.value("dbPassword", "a").toString();
    dbUsernameEdit->setText(dbUsername);
    dbPasswordEdit->setText(dbPassword);
}

void SettingsDialog::saveSettings()
{
    // Save settings to QSettings
    QSettings settings("YourCompany", "GameManager");

    // Write a machine-readable config file for the Python backend
    QJsonObject dbConfig;
    dbConfig["host"] = "127.0.0.1";
    dbConfig["port"] = "5432";
    dbConfig["database"] = "playerdata";
    dbConfig["user"] = dbUsernameEdit->text();
    dbConfig["password"] = dbPasswordEdit->text();

    QJsonDocument doc(dbConfig);
    QFile configFile("debug/backend/db_config.json");
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        configFile.write(doc.toJson());
        configFile.close();
        qDebug() << "Wrote debug/backend/db_config.json for Python backend.";
    }

    // Save theme
    settings.setValue("theme", currentTheme);

    // Save paths
    settings.setValue("recordingPath", recordingPath);
    settings.setValue("exportPath", exportPath);

    // Save data collection setting
    settings.setValue("enableDataCollection", enableDataCollection);

    settings.setValue("dbUsername", dbUsernameEdit->text());
    settings.setValue("dbPassword", dbPasswordEdit->text());
}

void SettingsDialog::applySettings()
{
    // Apply theme
    if (currentTheme == "Dark") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
    } else if (currentTheme == "Light") {
        QApplication::setStyle("Fusion");
        qApp->setPalette(QApplication::style()->standardPalette());
    } else {
        // Default theme
        QApplication::setStyle(QStyleFactory::create("windowsvista"));
    }

    // Create directories if they don't exist
    QDir dir;
    if (!dir.exists(recordingPath)) {
        dir.mkpath(recordingPath);
    }

    if (!dir.exists(exportPath)) {
        dir.mkpath(exportPath);
    }
}

void SettingsDialog::onCategoryChanged(int index)
{
    stackedWidget->setCurrentIndex(index);
}

void SettingsDialog::onOkClicked()
{
    // Save and apply settings
    currentTheme = themeComboBox->currentText();
    recordingPath = recordingPathEdit->text();
    exportPath = exportPathEdit->text();
    enableDataCollection = enableDataCollectionCheckBox->isChecked();

    saveSettings();
    applySettings();

    accept(); // Close the dialog
}

void SettingsDialog::onCancelClicked()
{
    reject(); // Close the dialog without saving
}

void SettingsDialog::onApplyClicked()
{
    // Save and apply settings
    currentTheme = themeComboBox->currentText();
    recordingPath = recordingPathEdit->text();
    exportPath = exportPathEdit->text();
    enableDataCollection = enableDataCollectionCheckBox->isChecked();

    saveSettings();
    applySettings();

    QMessageBox::information(this, "Settings", "Settings applied successfully!");
}

void SettingsDialog::onBrowseRecordingPath()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select Recording Path", recordingPath);
    if (!path.isEmpty()) {
        recordingPathEdit->setText(path);
    }
}

void SettingsDialog::onBrowseExportPath()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select Export Path", exportPath);
    if (!path.isEmpty()) {
        exportPathEdit->setText(path);
    }
}

void SettingsDialog::onThemeChanged(const QString &theme)
{
    // This is just for preview, actual theme change happens on apply
    currentTheme = theme;
}

void SettingsDialog::onExportDataClicked()
{
    // For now, let's just export data for the first player in the list.
    // In a real app, you'd have a selection dropdown.
    bool ok;
    QString playerName = QInputDialog::getText(this, "Export Data",
                                               "Enter the exact player name to export:",
                                               QLineEdit::Normal,
                                               "", &ok);
    if (!ok || playerName.isEmpty()) {
        QMessageBox::information(this, "Export Canceled", "No player name entered.");
        return;
    }

    // Call the function that was passed from MainWindow
    if (m_exportDataFunction) {
        // FIX: Call the function with QString arguments, not std::string.
        // The lambda in MainWindow handles the Python types and returns a QString.
        QString result_qstring = m_exportDataFunction(playerName, exportPath);
        QMessageBox::information(this, "Export Complete", result_qstring);
    } else {
        QMessageBox::critical(this, "Error", "The export function is not available.");
    }
}
