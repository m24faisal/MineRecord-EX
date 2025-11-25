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
INCLUDEPATH += $$PYTHON_PATH/include $$PYBIND11_PATH
LIBS += -L$$PYTHON_PATH/libs -lpython$$PYTHON_VERSION

backend_copy.target = copy_backend
backend_copy.commands = $(COPY_DIR) \"$$PWD/../backend\" \"$$OUT_PWD/\"
backend_copy.depends = first

QMAKE_EXTRA_TARGETS += backend_copy

