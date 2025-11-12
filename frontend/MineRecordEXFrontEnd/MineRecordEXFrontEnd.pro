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
