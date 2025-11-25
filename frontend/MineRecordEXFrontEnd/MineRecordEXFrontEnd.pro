QT       += core gui widgets

TARGET = GameManager
TEMPLATE = app

SOURCES += \
    infodialog.cpp \
    main.cpp \
    mainwindow.cpp \
    programinfo.cpp \
    processdetector.cpp \
    settingsdialog.cpp

HEADERS += \
    infodialog.h \
    mainwindow.h \
    programinfo.h \
    processdetector.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

PYBIND11_PATH = C:/Users/Mahir/AppData/Local/Programs/Python/Python313/Lib/site-packages/pybind11/include
PYTHON_PATH = C:\Users\Mahir\AppData\Local\Programs\Python\Python313
PYTHON_VERSION = 313
INCLUDEPATH += $$PYTHON_PATH/include
LIBS += -L$$PYTHON_PATH/libs -lpython$$PYTHON_VERSION

