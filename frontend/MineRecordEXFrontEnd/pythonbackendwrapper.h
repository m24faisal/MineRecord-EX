#ifndef PYTHONBACKENDWRAPPER_H
#define PYTHONBACKENDWRAPPER_H

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

    // Calls pybind11 to manage the data server inside the C++ process
    void startDataService();
    void stopDataService();

    // Calls pybind11 to manage screen recording
    std::string startRecording(const std::string& gameName, const std::string& gamePath, const std::string& recordingPath, bool enableDataCollection);
    std::string stopRecording(const std::string& recordingId);
    std::string exportPlayerData(const std::string& playerName, const std::string& exportPath);

private:
    // Helper method to access private implementation
    PythonBackendWrapperPrivate* d_ptr_cast() { return d_ptr; }

    // Pointer to the private implementation
    PythonBackendWrapperPrivate* d_ptr;
};

#endif // PYTHONBACKENDWRAPPER_H
