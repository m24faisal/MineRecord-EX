#ifndef PYTHONBACKENDWRAPPER_H
#define PYTHONBACKENDWRAPPER_H

#include <string>

// Forward declare private implementation struct
struct PythonBackendWrapperPrivate;

class PythonBackendWrapper
{
public:
    PythonBackendWrapper();
    ~PythonBackendWrapper();

    // Public interface for MainWindow to call
    void initialize();
    void shutdown();

    // Manages the standalone data receiver server (QProcess)
    void startDataService();
    void stopDataService();

    // Calls embedded Python functions via pybind11
    std::string startRecording(const std::string& gameName, const std::string& gamePath,
                               const std::string& recordingPath, bool enableDataCollection);
    std::string stopRecording(const std::string& recordingId);
    std::string exportPlayerData(const std::string& playerName, const std::string& exportPath);

private:
    // Helper to access private implementation
    PythonBackendWrapperPrivate* d_ptr_cast() { return d_ptr; }

    // Pointer to private implementation (PIMPL)
    PythonBackendWrapperPrivate* d_ptr;
};

// Global shutdown flag to prevent Python calls during finalization
extern bool g_pythonShuttingDown;

#endif // PYTHONBACKENDWRAPPER_H
