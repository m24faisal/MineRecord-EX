#ifndef PROCESSDETECTOR_H
#define PROCESSDETECTOR_H

#include "qdatetime.h"
#include <QString>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QObject>

class ProcessDetector : public QObject
{
    Q_OBJECT

public:
    static ProcessDetector& instance();

    bool isProcessRunning(const QString &processName);
    QList<QString> getRunningProcessNames();

    // Allow changing the update interval
    void setUpdateInterval(int milliseconds);
    int updateInterval() const;

    // Force an immediate update
    void forceUpdate();

signals:
    void processListUpdated();

private slots:
    void updateCache();

private:
    ProcessDetector();
    ~ProcessDetector();

    QMap<QString, bool> processCache;
    QDateTime lastCacheUpdate;
    QTimer *updateTimer;
    int m_updateInterval;

    // Platform-specific implementations
    void updateCacheWindows();
    void updateCacheLinux();
    void updateCacheMac();
};

#endif // PROCESSDETECTOR_H
