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
    QProcess* writerProcess;
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
        py::exec("import sys; sys.path.append('./backend')");
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
    if (d->serverProcess && d->serverProcess->state() == QProcess::Running) {
        qDebug() << "Data collection processes are already running.";
        return;
    }

    // --- Start the Standalone Server ---
    d->serverProcess = new QProcess();
    QString serverScript = QDir::currentPath() + "/backend/data_receiver_server.py";
    d->serverProcess->start("python", QStringList() << serverScript);
    if (!d->serverProcess->waitForStarted()) {
        qCritical() << "Failed to start server process:" << d->serverProcess->errorString();
        delete d->serverProcess;
        d->serverProcess = nullptr;
        return;
    }
    qDebug() << "Standalone Server process started with PID:" << d->serverProcess->processId();

    // --- Start the Database Writer ---
    d->writerProcess = new QProcess();
    QString writerScript = QDir::currentPath() + "/backend/db_writer.py";
    d->writerProcess->start("python", QStringList() << writerScript);
    if (!d->writerProcess->waitForStarted()) {
        qCritical() << "Failed to start writer process:" << d->writerProcess->errorString();
        delete d->writerProcess;
        d->writerProcess = nullptr;
        // If writer fails, we should stop the server too
        if(d->serverProcess) { d->serverProcess->kill(); d->serverProcess->waitForFinished(); delete d->serverProcess; d->serverProcess = nullptr; }
        return;
    }
    qDebug() << "Database Writer process started with PID:" << d->writerProcess->processId();
}

void PythonBackendWrapper::stopDataService()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (d->serverProcess) {
        qDebug() << "Stopping server process...";
        d->serverProcess->terminate();
        if (!d->serverProcess->waitForFinished(5000)) {
            d->serverProcess->kill();
            d->serverProcess->waitForFinished();
        }
        delete d->serverProcess;
        d->serverProcess = nullptr;
    }

    if (d->writerProcess) {
        qDebug() << "Stopping writer process...";
        d->writerProcess->terminate();
        if (!d->writerProcess->waitForFinished(5000)) {
            d->writerProcess->kill();
            d->writerProcess->waitForFinished();
        }
        delete d->writerProcess;
        d->writerProcess = nullptr;
    }
    qDebug() << "Data collection processes stopped.";
}

std::string PythonBackendWrapper::startRecording(const std::string& gameName, const std::string& gamePath, const std::string& recordingPath, bool enableDataCollection)
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return "Error: Python backend is not initialized.";
    try {
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
        py::object result = d->backend_module.attr("stop_recording")(recordingId);
        return result.cast<std::string>();
    } catch (const py::error_already_set &e) {
        return std::string("Python Error: ") + e.what();
    }
}

std::string PythonBackendWrapper::exportPlayerData(const std::string& playerName, const std::string& exportPath)
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return "Error: Python backend is not initialized.";
    try {
        py::object result = d->backend_module.attr("export_player_data")(playerName, exportPath);
        return result.cast<std::string>();
    } catch (const py::error_already_set &e) {
        return std::string("Python Error: ") + e.what();
    }
}
