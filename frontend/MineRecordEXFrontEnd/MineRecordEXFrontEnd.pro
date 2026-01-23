QT       += core gui widgets

TARGET = MineRecordEX
TEMPLATE = app

SOURCES += \
    infodialog.cpp \
    main.cpp \
    mainwindow.cpp \
    programinfo.cpp \
    processdetector.cpp \
    python_interpreter.cpp \
    pythonbackendwrapper.cpp \
    settingsdialog.cpp

HEADERS += \
    infodialog.h \
    mainwindow.h \
    programinfo.h \
    processdetector.h \
    pythonbackendwrapper.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

PYBIND11_PATH = C:/Users/Mahir/AppData/Local/Programs/Python/Python313/Lib/site-packages/pybind11/include
PYTHON_PATH = C:/Users\Mahir/AppData/Local/Programs/Python/Python313
PYTHON_VERSION = 313
INCLUDEPATH += $$PYTHON_PATH/include $$PYBIND11_PATH
LIBS += -L$$PYTHON_PATH/libs -lpython$$PYTHON_VERSION

win32 {
    QMAKE_POST_LINK += \"$$PWD/copy_files.bat\" \"$$OUT_PWD/debug\"
}
