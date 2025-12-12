#ifndef PYTHONBACKENDWRAPPER_H
#define PYTHONBACKENDWRAPPER_H

// --- NO QT HEADERS AND NO PYBIND11 HEADERS IN THIS FILE ---
// This file is now a pure C++ interface.

#include <string>

// Forward declare a private implementation struct
struct PythonBackendWrapperPrivate;

class PythonBackendWrapper
{
public:
    PythonBackendWrapper();
    ~PythonBackendWrapper();

    // Public interface for MainWindow to call
    void initialize();
    void shutdown();
    std::string startRecording(const std::string& gameName, const std::string& gamePath, const std::string& recordingPath, bool enableDataCollection);
    std::string stopRecording(const std::string& recordingId);
    void startDataService(bool enabled);
    void stopDataService();
    std::string exportPlayerData(const std::string& playerName, const std::string& exportPath);

private:
    // Pointer to the private implementation
    PythonBackendWrapperPrivate* d_ptr;

// --- FIX: Define our own macro to access the private implementation ---
// This replaces the Qt-specific Q_DECLARE_PRIVATE and Q_D macros.
#define d_ptr_cast() (d_ptr)
};

#endif // PYTHONBACKENDWRAPPER_H
