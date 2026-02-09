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
#include <QPalette>
#include "mainwindow.h"

SettingsDialog::SettingsDialog(MainWindow *mainWin, QWidget *parent, std::function<QString(const QString&, const QString&)> exportFunc)
    : QDialog(parent),
    m_MainWindow(mainWin),
    m_exportDataFunction(exportFunc)
{
    setupUI();
    loadSettings();
    setWindowTitle("Settings");
    setFixedSize(600, 480);
    setModal(true);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    contentLayout = new QHBoxLayout();

    categoryList = new QListWidget(this);
    categoryList->addItem("General");
    categoryList->addItem("Paths");
    categoryList->addItem("DB Settings");
    categoryList->setCurrentRow(0);
    categoryList->setMaximumWidth(150);

    // Initialize with current theme from QSettings
    QSettings settings("Stat Tracker", "MineRecordEX");
    QString initialTheme = settings.value("theme", "Light").toString();
    updateSidebarTheme(initialTheme);

    stackedWidget = new QStackedWidget(this);
    setupGeneralSettings();
    setupPathSettings();
    setupDbSettings();
    stackedWidget->addWidget(generalSettingsWidget);
    stackedWidget->addWidget(pathSettingsWidget);
    stackedWidget->addWidget(dbSettingsWidget);

    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    applyButton = new QPushButton("Apply", this);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);

    contentLayout->addWidget(categoryList);
    contentLayout->addWidget(stackedWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addLayout(contentLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(9, 9, 9, 9);

    connect(categoryList, &QListWidget::currentRowChanged, this, &SettingsDialog::onCategoryChanged);
    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
    connect(applyButton, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
    connect(recordingBrowseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseRecordingPath);
    connect(exportBrowseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseExportPath);
    // REMOVED: connect(themeComboBox, &QComboBox::currentTextChanged, this, &SettingsDialog::onThemeChanged);
    connect(exportDataButton, &QPushButton::clicked, this, &SettingsDialog::onExportDataClicked);
}

void SettingsDialog::setupGeneralSettings()
{
    generalSettingsWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(generalSettingsWidget);

    QGroupBox *themeGroup = new QGroupBox("Theme", this);
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroup);

    QHBoxLayout *themeSelectLayout = new QHBoxLayout();
    QLabel *themeLabel = new QLabel("Application Theme:", this);
    themeComboBox = new QComboBox(this);

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

    QGroupBox *recordingGroup = new QGroupBox("Recording Path", this);
    QVBoxLayout *recordingLayout = new QVBoxLayout(recordingGroup);

    QHBoxLayout *recordingPathLayout = new QHBoxLayout();
    QLabel *recordingLabel = new QLabel("Recording Save Path:", this);
    recordingPathEdit = new QLineEdit(this);
    recordingBrowseButton = new QPushButton("Browse...", this);

    recordingPathEdit->setText(QCoreApplication::applicationDirPath() + "/GameRecordings");

    recordingPathLayout->addWidget(recordingLabel);
    recordingPathLayout->addWidget(recordingPathEdit);
    recordingPathLayout->addWidget(recordingBrowseButton);

    recordingLayout->addWidget(recordingLabel);
    recordingLayout->addLayout(recordingPathLayout);

    QGroupBox *exportGroup = new QGroupBox("Export Path", this);
    QVBoxLayout *exportLayout = new QVBoxLayout(exportGroup);

    QHBoxLayout *exportPathLayout = new QHBoxLayout();
    QLabel *exportLabel = new QLabel("Export Data Path:", this);
    exportPathEdit = new QLineEdit(this);
    exportBrowseButton = new QPushButton("Browse...", this);

    exportPathEdit->setText(QCoreApplication::applicationDirPath() + "/GameExports");

    exportPathLayout->addWidget(exportLabel);
    exportPathLayout->addWidget(exportPathEdit);
    exportPathLayout->addWidget(exportBrowseButton);

    exportLayout->addWidget(exportLabel);
    exportLayout->addLayout(exportPathLayout);

    QGroupBox *dataCollectionGroup = new QGroupBox("Data Collection", this);
    QVBoxLayout *dataCollectionLayout = new QVBoxLayout(dataCollectionGroup);

    enableDataCollectionCheckBox = new QCheckBox("Enable data collection", this);
    enableDataCollectionCheckBox->setToolTip("When checked, game data will be collected through data_receiver.py during recording");

    dataCollectionLayout->addWidget(enableDataCollectionCheckBox);

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

    dbHostEdit = new QLineEdit(this);
    dbPortEdit = new QLineEdit(this);
    dbUsernameEdit = new QLineEdit(this);
    dbPasswordEdit = new QLineEdit(this);
    dbDatabaseEdit = new QLineEdit(this);
    dbPasswordEdit->setEchoMode(QLineEdit::Password);

    dbHostEdit->setText("127.0.0.1");
    dbPortEdit->setText("5432");
    dbDatabaseEdit->setText("playerdata");
    dbUsernameEdit->setText("postgres");
    dbPasswordEdit->setText("postgres");

    formLayout->addRow("Host:", dbHostEdit);
    formLayout->addRow("Port:", dbPortEdit);
    formLayout->addRow("Database:", dbDatabaseEdit);
    formLayout->addRow("Username:", dbUsernameEdit);
    formLayout->addRow("Password:", dbPasswordEdit);

    layout->addWidget(dbGroup);
    layout->addStretch();
}

void SettingsDialog::loadSettings()
{
    QSettings settings("Stat Tracker", "MineRecordEX");

    // REMOVED: currentTheme = settings.value("theme", "Light").toString();
    QString currentTheme = settings.value("theme", "Light").toString(); // Local variable only
    int themeIndex = themeComboBox->findText(currentTheme);
    if (themeIndex >= 0) {
        themeComboBox->setCurrentIndex(themeIndex);
    }

    QString appDir = QCoreApplication::applicationDirPath();

    recordingPath = settings.value("recordingPath", appDir + "/GameRecordings").toString();
    recordingPathEdit->setText(recordingPath);

    exportPath = settings.value("exportPath", appDir + "/GameExports").toString();
    exportPathEdit->setText(exportPath);

    enableDataCollection = settings.value("enableDataCollection", false).toBool();
    enableDataCollectionCheckBox->setChecked(enableDataCollection);

    QString dbHost = settings.value("dbHost", "127.0.0.1").toString();
    QString dbPort = settings.value("dbPort", "5432").toString();
    QString dbDatabase = settings.value("dbDatabase", "playerdata").toString();
    QString dbUsername = settings.value("dbUsername", "postgres").toString();
    QString dbPassword = settings.value("dbPassword", "postgres").toString();

    dbHostEdit->setText(dbHost);
    dbPortEdit->setText(dbPort);
    dbDatabaseEdit->setText(dbDatabase);
    dbUsernameEdit->setText(dbUsername);
    dbPasswordEdit->setText(dbPassword);
}

void SettingsDialog::saveSettings()
{
    QSettings settings("Stat Tracker", "MineRecordEX");
    // REMOVED: settings.setValue("theme", currentTheme);
    settings.setValue("theme", themeComboBox->currentText()); // Get current selection directly
    settings.setValue("recordingPath", recordingPath);
    settings.setValue("exportPath", exportPath);
    settings.setValue("enableDataCollection", enableDataCollection);
    settings.setValue("dbHost", dbHostEdit->text());
    settings.setValue("dbPort", dbPortEdit->text());
    settings.setValue("dbDatabase", dbDatabaseEdit->text());
    settings.setValue("dbUsername", dbUsernameEdit->text());
    settings.setValue("dbPassword", dbPasswordEdit->text());

    // Ensure backend directory exists and save config there
    QString backendDir = QCoreApplication::applicationDirPath() + "/backend";
    QDir().mkpath(backendDir);

    QJsonObject dbConfig;
    dbConfig["host"] = dbHostEdit->text();
    dbConfig["port"] = dbPortEdit->text();
    dbConfig["database"] = dbDatabaseEdit->text();
    dbConfig["user"] = dbUsernameEdit->text();
    dbConfig["password"] = dbPasswordEdit->text();

    QJsonDocument doc(dbConfig);
    QString configPath = backendDir + "/db_config.json";
    QFile configFile(configPath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        configFile.write(doc.toJson());
        configFile.close();
        qDebug() << "Wrote db_config.json to:" << configPath;
    }
}

void SettingsDialog::applySettings()
{
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

void SettingsDialog::updateSidebarTheme(const QString &theme)
{
    if (theme == "Dark") {
        QPalette pal = categoryList->palette();
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::HighlightedText, Qt::white);
        categoryList->setPalette(pal);

        categoryList->setStyleSheet(
            "QListWidget {background-color:transparent; border:none; outline:none; padding:0; margin:0;}"
            "QListWidget::item {padding:8px 16px; color:white; background:transparent;}"
            "QListWidget::item:selected {background-color:#2E8B57; color:white;}"
            "QListWidget::item:hover {background-color:#3A5F4A; color:white;}"
            );
    } else {
        QPalette pal = categoryList->palette();
        pal.setColor(QPalette::Text, Qt::black);
        pal.setColor(QPalette::HighlightedText, Qt::white);
        categoryList->setPalette(pal);

        categoryList->setStyleSheet(
            "QListWidget {background-color:transparent; border:none; outline:none; padding:0; margin:0;}"
            "QListWidget::item {padding:8px 16px; color:black; background:transparent;}"
            "QListWidget::item:selected {background-color:#2E8B57; color:white;}"
            "QListWidget::item:hover {background-color:#3A5F4A; color:black;}"
            );
    }
}

void SettingsDialog::onApplyClicked()
{
    applySettingsInternal();
}

void SettingsDialog::onOkClicked()
{
    applySettingsInternal();
    QMessageBox::information(this, "Settings", "All settings applied successfully!");
    accept();
}

void SettingsDialog::applySettingsInternal()
{
    QString selectedTheme = themeComboBox->currentText();
    QString recordingPath = recordingPathEdit->text();
    QString exportPath = exportPathEdit->text();
    bool enableDataCollection = enableDataCollectionCheckBox->isChecked();
    QString dbHost = dbHostEdit->text();
    QString dbPort = dbPortEdit->text();
    QString dbDatabase = dbDatabaseEdit->text();
    QString dbUsername = dbUsernameEdit->text();
    QString dbPassword = dbPasswordEdit->text();

    QSettings settings("Stat Tracker", "MineRecordEX");
    settings.setValue("theme", selectedTheme);
    settings.setValue("recordingPath", recordingPath);
    settings.setValue("exportPath", exportPath);
    settings.setValue("enableDataCollection", enableDataCollection);
    settings.setValue("dbHost", dbHost);
    settings.setValue("dbPort", dbPort);
    settings.setValue("dbDatabase", dbDatabase);
    settings.setValue("dbUsername", dbUsername);
    settings.setValue("dbPassword", dbPassword);

    // Save db_config.json to backend directory
    QString backendDir = QCoreApplication::applicationDirPath() + "/backend";
    QDir().mkpath(backendDir);

    QJsonObject dbConfig;
    dbConfig["host"] = dbHost;
    dbConfig["port"] = dbPort;
    dbConfig["database"] = dbDatabase;
    dbConfig["user"] = dbUsername;
    dbConfig["password"] = dbPassword;

    QJsonDocument doc(dbConfig);
    QString configPath = backendDir + "/db_config.json";
    QFile configFile(configPath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        configFile.write(doc.toJson());
        configFile.close();
    }

    updateSidebarTheme(selectedTheme);

    // Apply to main window
    if (m_MainWindow) {
        m_MainWindow->applySettings();
    }
}

void SettingsDialog::onCancelClicked()
{
    reject();
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

// REMOVED: onThemeChanged() method entirely

void SettingsDialog::onExportDataClicked()
{
    bool ok;
    QString playerName = QInputDialog::getText(this, "Export Data",
                                               "Enter the exact player name to export:",
                                               QLineEdit::Normal,
                                               "", &ok);
    if (!ok || playerName.isEmpty()) {
        QMessageBox::information(this, "Export Canceled", "No player name entered.");
        return;
    }

    if (m_exportDataFunction) {
        QString result_qstring = m_exportDataFunction(playerName, exportPath);
        QMessageBox::information(this, "Export Complete", result_qstring);
    } else {
        QMessageBox::critical(this, "Error", "The export function is not available.");
    }
}
