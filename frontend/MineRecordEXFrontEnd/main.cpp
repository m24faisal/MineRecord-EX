#include "mainwindow.h"
#include <QApplication>

// --- ALL PYBIND11 CODE HAS BEEN REMOVED FROM THIS FILE ---
// The interpreter is now started automatically by the static variable
// in python_interpreter.cpp when the program loads.

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
