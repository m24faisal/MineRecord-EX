#include "processdetector.h"
#include <QProcess>
#include <QDateTime>
#include <QDebug>

ProcessDetector& ProcessDetector::instance()
{
    static ProcessDetector instance;
    return instance;
}

ProcessDetector::ProcessDetector()
    : m_updateInterval(1000) // Default to 1 second
{
    // Set up the timer for automatic updates
    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateCache()));
    updateTimer->start(m_updateInterval);

    // Initialize the cache
    updateCache();
}

ProcessDetector::~ProcessDetector()
{
}

bool ProcessDetector::isProcessRunning(const QString &processName)
{
    // Check if the process is in the cache
    QString normalizedName = processName.toLower();

    // Remove .exe extension for Windows processes
    if (normalizedName.endsWith(".exe")) {
        normalizedName = normalizedName.left(normalizedName.length() - 4);
    }

    return processCache.value(normalizedName, false);
}

QList<QString> ProcessDetector::getRunningProcessNames()
{
    return processCache.keys();
}

void ProcessDetector::setUpdateInterval(int milliseconds)
{
    if (milliseconds < 100) {
        milliseconds = 100; // Minimum 100ms to avoid excessive CPU usage
    }

    m_updateInterval = milliseconds;
    updateTimer->setInterval(m_updateInterval);
}

int ProcessDetector::updateInterval() const
{
    return m_updateInterval;
}

void ProcessDetector::forceUpdate()
{
    updateCache();
}

void ProcessDetector::updateCache()
{
    processCache.clear();

#ifdef Q_OS_WIN
    updateCacheWindows();
#elif defined(Q_OS_LINUX)
    updateCacheLinux();
#elif defined(Q_OS_MAC)
    updateCacheMac();
#endif

    lastCacheUpdate = QDateTime::currentDateTime();
    emit processListUpdated();
}

void ProcessDetector::updateCacheWindows()
{
    // Use wmic for faster process listing on Windows
    QProcess wmic;
    wmic.start("wmic", QStringList() << "process" << "get" << "name" << "/FORMAT:csv");
    wmic.waitForFinished(500); // Wait max 500ms

    if (wmic.exitCode() != 0) {
        // Fallback to tasklist if wmic fails
        QProcess tasklist;
        tasklist.start("tasklist", QStringList() << "/FO" << "CSV");
        tasklist.waitForFinished(500);

        QString output = tasklist.readAllStandardOutput();
        QStringList lines = output.split('\n');

        // Skip the header line
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;

            // Parse CSV line
            QStringList fields = line.split(',');
            if (fields.size() >= 2) {
                // Remove quotes from process name
                QString processName = fields[0].mid(1, fields[0].length() - 2).toLower();

                // Remove .exe extension
                if (processName.endsWith(".exe")) {
                    processName = processName.left(processName.length() - 4);
                }

                processCache[processName] = true;
            }
        }
    } else {
        QString output = wmic.readAllStandardOutput();
        QStringList lines = output.split('\n');

        // Skip header lines
        for (int i = 2; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;

            // Parse CSV line
            QStringList fields = line.split(',');
            if (fields.size() >= 2) {
                QString processName = fields[1].toLower();

                // Remove .exe extension
                if (processName.endsWith(".exe")) {
                    processName = processName.left(processName.length() - 4);
                }

                processCache[processName] = true;
            }
        }
    }
}

void ProcessDetector::updateCacheLinux()
{
    // Use ps with specific columns for faster processing
    QProcess ps;
    ps.start("ps", QStringList() << "-e" << "-o" << "comm=");
    ps.waitForFinished(500); // Wait max 500ms

    QString output = ps.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        QString processName = line.trimmed().toLower();
        if (!processName.isEmpty()) {
            processCache[processName] = true;
        }
    }
}

void ProcessDetector::updateCacheMac()
{
    // Use ps with specific columns for faster processing
    QProcess ps;
    ps.start("ps", QStringList() << "-e" << "-o" << "comm=");
    ps.waitForFinished(500); // Wait max 500ms

    QString output = ps.readAllStandardOutput();
    QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        QString processName = line.trimmed().toLower();
        if (!processName.isEmpty()) {
            processCache[processName] = true;
        }
    }
}
