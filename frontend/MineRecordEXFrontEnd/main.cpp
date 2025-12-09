#include "mainwindow.h"
#include <QApplication>

// --- CRITICAL: Include pybind11 here ---
#include <pybind11/embed.h>
namespace py = pybind11;

int main(int argc, char *argv[])
{
    // --- CRITICAL: Initialize the Python interpreter ONCE for the entire application ---
    py::scoped_interpreter guard{};

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
