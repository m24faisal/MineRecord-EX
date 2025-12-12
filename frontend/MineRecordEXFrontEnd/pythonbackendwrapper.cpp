#include "pythonbackendwrapper.h"
// --- CRITICAL FIX: REMOVED #include <QDebug> ---
// We now use standard C++ iostream for logging to avoid any Qt/pybind11 conflict.

#include <iostream> // For std::cout, std::cerr, std::endl

// --- THIS IS THE ONLY PLACE WHERE PYBIND11 IS INCLUDED ---
#include <pybind11/embed.h>
namespace py = pybind11;

// --- The private implementation struct holds all pybind11 members ---
struct PythonBackendWrapperPrivate {
    py::module backend_module;
    bool isInitialized = false;
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
        // --- FIX: Replaced qDebug() with std::cout ---
        std::cout << "SUCCESS: Python backend initialized via wrapper." << std::endl;
    } catch (const py::error_already_set &e) {
        // --- FIX: Replaced qCritical() with std::cerr ---
        std::cerr << "Python initialization failed: " << e.what() << std::endl;
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
        d->backend_module.attr("stop_data_collection_service")();
        d->backend_module.attr("shutdown_all")();
        // --- FIX: Replaced qDebug() with std::cout ---
        std::cout << "SUCCESS: Python backend shutdown via wrapper." << std::endl;
    } catch (const py::error_already_set &e) {
        // --- FIX: Replaced qWarning() with std::cerr ---
        std::cerr << "Error during Python shutdown: " << e.what() << std::endl;
    }
    d->isInitialized = false;
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

void PythonBackendWrapper::startDataService(bool enabled)
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return;
    try {
        d->backend_module.attr("start_data_collection_service")(enabled);
    } catch (const py::error_already_set &e) {
        // --- FIX: Replaced qCritical() with std::cerr ---
        std::cerr << "Failed to start data collection service: " << e.what() << std::endl;
    }
}

void PythonBackendWrapper::stopDataService()
{
    PythonBackendWrapperPrivate *d = d_ptr_cast();
    if (!d->isInitialized) return;
    try {
        d->backend_module.attr("stop_data_collection_service")();
    } catch (const py::error_already_set &e) {
        // --- FIX: Replaced qWarning() with std::cerr ---
        std::cerr << "Error stopping data collection service: " << e.what() << std::endl;
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
