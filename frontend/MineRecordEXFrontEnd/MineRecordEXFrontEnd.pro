QT += core gui widgets

TARGET = MineRecordEX
TEMPLATE = app

SOURCES += \
    infodialog.cpp \
    main.cpp \
    mainwindow.cpp \
    programinfo.cpp \
    processdetector.cpp \
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

# Auto-detect Python installation (privacy-safe)
PYTHON_VERSION = 313

# Try common Python installation locations
exists(C:/Python$$PYTHON_VERSION) {
    PYTHON_PATH = C:/Python$$PYTHON_VERSION
} else {
    exists($$quote($$system(echo %LOCALAPPDATA%))/Programs/Python/Python$$PYTHON_VERSION) {
        PYTHON_PATH = $$quote($$system(echo %LOCALAPPDATA%))/Programs/Python/Python$$PYTHON_VERSION
    } else {
        error("Python $$PYTHON_VERSION not found. Please install Python $$PYTHON_VERSION.")
    }
}

PYBIND11_PATH = $$PYTHON_PATH/Lib/site-packages/pybind11/include

INCLUDEPATH += $$PYTHON_PATH/include $$PYBIND11_PATH
LIBS += -L$$PYTHON_PATH/libs -lpython$$PYTHON_VERSION

win32 {
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK += \"$$PWD/copy_files.bat\" \"$$OUT_PWD/debug\"
    } else {
        QMAKE_POST_LINK += \"$$PWD/copy_files.bat\" \"$$OUT_PWD/release\"
    }
    RC_FILE = application.rc
}
