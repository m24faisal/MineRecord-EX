// pythonbackendwrapper.cpp
#include "pythonbackendwrapper.h"

// --- INCLUDES IN THE CORRECT ORDER ---
// 1. Include pybind11 headers first.
#include <pybind11/embed.h>
namespace py = pybind11;

// 2. THEN include Qt headers.
#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QThread>
#include <memory>

// Global shutdown flag
bool g_pythonShuttingDown = false;

// --- The private implementation struct holds ALL members, both pybind11 and Qt ---
struct PythonBackendWrapperPrivate {
    py::module backend_module;
    bool isInitialized = false;
    QProcess* serverProcess = nullptr;
    std::unique_ptr<py::scoped_interpreter> interpreter; // Manages Python interpreter lifecycle
};

PythonBackendWrapper::PythonBackendWrapper()
    : d_ptr(new PythonBackendWrapperPrivate())
{
    // Initialize Python interpreter in constructor (after Qt is ready)
    d_ptr->interpreter = std::make_unique<py::scoped_interpreter>();
}

PythonBackendWrapper::~PythonBackendWrapper()
{
    // DO NOT call shutdown() here - handled by MainWindow::cleanupBeforeQuit()
    delete d_ptr;
}

void PythonBackendWrapper::initialize()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    try {
        // Use application directory (where .exe actually runs)
        QString appDir = QCoreApplication::applicationDirPath();
        QString backendPath = appDir + "/backend";
        QString pythonPath = appDir + "/python";

        qDebug() << "Backend Path:" << backendPath;
        qDebug() << "Python Path:" << pythonPath;

        // Configure Python to use bundled installation
        py::exec("import sys");
        py::exec("sys.executable = '" + pythonPath.toStdString() + "/python.exe'");
        py::exec("sys.prefix = sys.exec_prefix = '" + pythonPath.toStdString() + "'");
        py::exec("sys.path.insert(0, '" + backendPath.toStdString() + "')");
        py::exec("sys.path.insert(0, '" + pythonPath.toStdString() + "/Lib/site-packages')");

        // Import the main controller module
        d->backend_module = py::module::import("backend_controller");
        d->isInitialized = true;
        qDebug() << "SUCCESS: Python backend initialized via wrapper.";
    } catch (const py::error_already_set &e) {
        qCritical() << "Python initialization failed:" << e.what();
        d->isInitialized = false;
    }
}

void PythonBackendWrapper::shutdown()
{
    // SET SHUTDOWN FLAG FIRST
    g_pythonShuttingDown = true;

    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) {
        return;
    }

    // Call Python shutdown function if module exists
    if (d->backend_module) {
        try {
            d->backend_module.attr("shutdown_all")();
            qDebug() << "SUCCESS: Python backend shutdown via wrapper.";
        } catch (...) {
            // Silently ignore errors during shutdown
        }
    }

    // Reset the module to release Python references
    d->backend_module = py::module();

    // Reset the interpreter AFTER all Python objects are destroyed
    d->interpreter.reset();

    d->isInitialized = false;
}

void PythonBackendWrapper::startDataService()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) {
        qCritical() << "Cannot start service: Python backend not initialized.";
        return;
    }
    // Check if the server process is already running
    if (d->serverProcess && d->serverProcess->state() == QProcess::Running) {
        qDebug() << "Data collection server is already running.";
        return;
    }
    // START THE STANDALONE DATA RECEIVER SERVER
    QString appDir = QCoreApplication::applicationDirPath();
    QString backendPath = appDir + "/backend";
    QString serverScript = backendPath + "/data_receiver.py";
    QString pythonExe = appDir + "/python/python.exe"; // Use bundled Python

    qDebug() << "Starting server process with script:" << serverScript;
    qDebug() << "Using Python executable:" << pythonExe;

    d->serverProcess = new QProcess();
    d->serverProcess->start(pythonExe, QStringList() << serverScript);
    if (!d->serverProcess->waitForStarted()) {
        qCritical() << "Failed to start server process:" << d->serverProcess->errorString();
        delete d->serverProcess;
        d->serverProcess = nullptr;
        return;
    }
    qDebug() << "Data Receiver Server process started with PID:" << d->serverProcess->processId();
}

void PythonBackendWrapper::stopDataService()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (d->serverProcess) {
        qDebug() << "Stopping data receiver server process...";

        // CREATE SHUTDOWN SIGNAL FILE
        QString shutdownFile = QCoreApplication::applicationDirPath() + "/data_receiver.shutdown";
        QFile file(shutdownFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("shutdown");
            file.close();
            qDebug() << "Shutdown signal file created:" << shutdownFile;
        }

        // Wait for graceful shutdown (up to 5 seconds)
        if (!d->serverProcess->waitForFinished(5000)) {
            qDebug() << "Server did not terminate gracefully, killing it.";
            d->serverProcess->kill();
            d->serverProcess->waitForFinished(2000);
        }

        // Clean up shutdown file if it still exists
        if (QFile::exists(shutdownFile)) {
            QFile::remove(shutdownFile);
        }

        delete d->serverProcess;
        d->serverProcess = nullptr;
    }
    qDebug() << "Data Receiver Server process stopped.";

    // ADD SMALL DELAY TO ENSURE PYTHON PROCESS FULLY EXITS
    QThread::msleep(50);
}

std::string PythonBackendWrapper::startRecording(const std::string& gameName, const std::string& gamePath,
                                                 const std::string& recordingPath, bool enableDataCollection)
{
    // CHECK SHUTDOWN FLAG FIRST
    if (g_pythonShuttingDown) {
        return "Error: Python is shutting down.";
    }

    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return "Error: Python backend is not initialized.";
    try {
        // Call the start_recording function in the backend controller
        py::object result = d->backend_module.attr("start_recording")(gameName, gamePath, recordingPath, enableDataCollection);
        return result.cast<std::string>();
    } catch (const py::error_already_set &e) {
        return std::string("Python Error: ") + e.what();
    }
}

std::string PythonBackendWrapper::stopRecording(const std::string& recordingId)
{
    // CHECK SHUTDOWN FLAG FIRST
    if (g_pythonShuttingDown) {
        return "Error: Python is shutting down.";
    }

    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return "Error: Python backend is not initialized.";
    try {
        // Call the stop_recording function in the backend controller
        py::object result = d->backend_module.attr("stop_recording")(recordingId);
        return result.cast<std::string>();
    } catch (const py::error_already_set &e) {
        return std::string("Python Error: ") + e.what();
    }
}

std::string PythonBackendWrapper::exportPlayerData(const std::string& playerName, const std::string& exportPath)
{
    // CHECK SHUTDOWN FLAG FIRST
    if (g_pythonShuttingDown) {
        return "Error: Python is shutting down.";
    }

    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) {
        qCritical() << "[WRAPPER] Cannot export: Python backend is not initialized.";
        return "Error: Python backend is not initialized.";
    }
    qDebug() << "[WRAPPER] Attempting to export data for player:" << QString::fromStdString(playerName);
    qDebug() << "[WRAPPER] Export path is:" << QString::fromStdString(exportPath);
    try {
        // Call the export_player_data function in the backend controller
        py::object result = d->backend_module.attr("export_player_data")(playerName, exportPath);
        std::string result_str = result.cast<std::string>();
        qDebug() << "[WRAPPER] Python call returned:" << QString::fromStdString(result_str);
        return result_str;
    } catch (const py::error_already_set &e) {
        QString errorMsg = "Python Error: " + QString::fromStdString(e.what());
        qCritical() << "[WRAPPER] Python exception occurred:" << errorMsg;
        return errorMsg.toStdString();
    }
}
