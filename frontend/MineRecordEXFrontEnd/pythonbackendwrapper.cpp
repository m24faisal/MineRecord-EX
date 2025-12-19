// pythonbackendwrapper.cpp
#include "pythonbackendwrapper.h"

// --- THIS IS THE CORRECT ORDER ---
// 1. Include pybind11 headers first.
#include <pybind11/embed.h>
namespace py = pybind11;

// 2. THEN include Qt headers.
#include <QDebug>
#include <QProcess>
#include <QDir>

// --- The private implementation struct holds ALL members, both pybind11 and Qt ---
struct PythonBackendWrapperPrivate {
    py::module backend_module;
    bool isInitialized = false;

    // --- QProcess is now declared here, inside the private struct ---
    QProcess* serverProcess;
};

PythonBackendWrapper::PythonBackendWrapper()
    : d_ptr(new PythonBackendWrapperPrivate())
{
}

PythonBackendWrapper::~PythonBackendWrapper()
{
    shutdown();
    delete d_ptr;
}

void PythonBackendWrapper::initialize()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    try {
        // Add the 'backend' directory to Python's path
        py::exec("import sys; sys.path.append('./backend')");
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
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized || !d->backend_module) {
        return;
    }
    try {
        // Call the shutdown_all function in the backend controller
        d->backend_module.attr("shutdown_all")();
        qDebug() << "SUCCESS: Python backend shutdown via wrapper.";
    } catch (const py::error_already_set &e) {
        qWarning() << "Error during Python shutdown:" << e.what();
    }
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

    // --- Start the Standalone Data Receiver Server ---
    // This process will run the data_receiver.py script
    d->serverProcess = new QProcess();
    QString serverScript = QDir::currentPath() + "/backend/data_receiver.py";

    qDebug() << "Starting server process with script:" << serverScript;
    d->serverProcess->start("python", QStringList() << serverScript);

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
        d->serverProcess->terminate();
        // Give it some time to close gracefully
        if (!d->serverProcess->waitForFinished(5000)) {
            qDebug() << "Server did not terminate, killing it.";
            d->serverProcess->kill();
            d->serverProcess->waitForFinished();
        }
        delete d->serverProcess;
        d->serverProcess = nullptr;
    }
    qDebug() << "Data Receiver Server process stopped.";
}

std::string PythonBackendWrapper::startRecording(const std::string& gameName, const std::string& gamePath, const std::string& recordingPath, bool enableDataCollection)
{
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
