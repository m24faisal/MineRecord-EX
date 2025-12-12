// --- CRITICAL: NO QT HEADERS IN THIS FILE ---
// This file's sole purpose is to manage the Python interpreter's lifecycle,
// completely isolated from Qt's macro definitions.

#include <pybind11/embed.h>

// --- FIX: Add the namespace alias for this translation unit ---
namespace py = pybind11;

// The pybind11 interpreter guard is defined as a static variable.
// It will be constructed when this variable is first accessed (which is
// effectively at program start) and destroyed at program exit.
// This is the cleanest way to ensure a single, application-wide instance.
static py::scoped_interpreter interpreter_guard;
