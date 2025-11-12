#ifndef PROGRAMINFO_H
#define PROGRAMINFO_H

#include <QString>
#include <QDateTime>

class ProgramInfo
{
public:
    ProgramInfo();
    ProgramInfo(const QString &name, const QString &path);

    QString name() const;
    void setName(const QString &name);

    QString path() const;
    void setPath(const QString &path);

    bool isRunning() const;
    void setRunning(bool running);

    qint64 timePlayedInSeconds() const;
    void setTimePlayedInSeconds(qint64 seconds);

    QString formattedTimePlayed() const;

    bool isRecording() const;
    void setRecording(bool recording);

private:
    QString m_name;
    QString m_path;
    bool m_running;
    bool m_recording;
    qint64 m_timePlayedInSeconds; // Total time played in seconds
    QDateTime m_lastStartTime;   // When the program was last started
    QDateTime m_recordingStartTime; // When recording was started
};

#endif // PROGRAMINFO_H
