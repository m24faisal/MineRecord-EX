#include "programinfo.h"
#include "processdetector.h"

ProgramInfo::ProgramInfo()
    : m_running(false), m_recording(false), m_timePlayedInSeconds(0)
{
}

ProgramInfo::ProgramInfo(const QString &name, const QString &path)
    : m_name(name), m_path(path), m_running(false), m_recording(false), m_timePlayedInSeconds(0)
{
    // Check if the program is currently running
    m_running = ProcessDetector::instance().isProcessRunning(name);
    if (m_running) {
        m_lastStartTime = QDateTime::currentDateTime();
    }
}

QString ProgramInfo::name() const
{
    return m_name;
}

void ProgramInfo::setName(const QString &name)
{
    m_name = name;
}

QString ProgramInfo::path() const
{
    return m_path;
}

void ProgramInfo::setPath(const QString &path)
{
    m_path = path;
}

bool ProgramInfo::isRunning() const
{
    return m_running;
}

void ProgramInfo::setRunning(bool running)
{
    if (m_running != running) {
        m_running = running;

        if (running) {
            // Program started running
            m_lastStartTime = QDateTime::currentDateTime();
        } else {
            // Program stopped running
            QDateTime now = QDateTime::currentDateTime();
            m_timePlayedInSeconds += m_lastStartTime.secsTo(now);

            // Also stop recording if it was active
            if (m_recording) {
                setRecording(false);
            }
        }
    }
}

qint64 ProgramInfo::timePlayedInSeconds() const
{
    qint64 totalTime = m_timePlayedInSeconds;

    // If currently running, add the time since it was started
    if (m_running) {
        QDateTime now = QDateTime::currentDateTime();
        totalTime += m_lastStartTime.secsTo(now);
    }

    return totalTime;
}

void ProgramInfo::setTimePlayedInSeconds(qint64 seconds)
{
    m_timePlayedInSeconds = seconds;
}

QString ProgramInfo::formattedTimePlayed() const
{
    qint64 totalSeconds = timePlayedInSeconds();
    qint64 hours = totalSeconds / 3600;
    qint64 minutes = (totalSeconds % 3600) / 60;
    qint64 seconds = totalSeconds % 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

bool ProgramInfo::isRecording() const
{
    return m_recording;
}

void ProgramInfo::setRecording(bool recording)
{
    if (m_recording != recording) {
        m_recording = recording;

        if (recording) {
            // Recording started
            m_recordingStartTime = QDateTime::currentDateTime();
        } else {
            // Recording stopped
            // Here you could save the recording or perform other actions
        }
    }
}
