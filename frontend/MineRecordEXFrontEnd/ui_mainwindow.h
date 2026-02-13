/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionSettings;
    QAction *actionAdd_Game;
    QAction *actionGitHub;
    QAction *actionInfo;
    QAction *actionExit_Application;
    QAction *fileActionStartRecording;
    QAction *fileActionStopRecording;
    QAction *actionRemove_Game;
    QAction *actionKo_Fi;
    QWidget *centralwidget;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuAbout;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        MainWindow->setStyleSheet(QString::fromUtf8("#centralwidget {\n"
"    background-color: black;\n"
"}\n"
""));
        actionSettings = new QAction(MainWindow);
        actionSettings->setObjectName("actionSettings");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/settings.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionSettings->setIcon(icon);
        actionAdd_Game = new QAction(MainWindow);
        actionAdd_Game->setObjectName("actionAdd_Game");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/AddPlus.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionAdd_Game->setIcon(icon1);
        actionGitHub = new QAction(MainWindow);
        actionGitHub->setObjectName("actionGitHub");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/github-mark-white.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionGitHub->setIcon(icon2);
        actionInfo = new QAction(MainWindow);
        actionInfo->setObjectName("actionInfo");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/Info.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionInfo->setIcon(icon3);
        actionExit_Application = new QAction(MainWindow);
        actionExit_Application->setObjectName("actionExit_Application");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/Close X.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionExit_Application->setIcon(icon4);
        fileActionStartRecording = new QAction(MainWindow);
        fileActionStartRecording->setObjectName("fileActionStartRecording");
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/startRecord.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        fileActionStartRecording->setIcon(icon5);
        fileActionStopRecording = new QAction(MainWindow);
        fileActionStopRecording->setObjectName("fileActionStopRecording");
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/stopRecording.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        fileActionStopRecording->setIcon(icon6);
        actionRemove_Game = new QAction(MainWindow);
        actionRemove_Game->setObjectName("actionRemove_Game");
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/trash.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionRemove_Game->setIcon(icon7);
        actionKo_Fi = new QAction(MainWindow);
        actionKo_Fi->setObjectName("actionKo_Fi");
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/ko-fi-svgrepo-com.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actionKo_Fi->setIcon(icon8);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        MainWindow->setCentralWidget(centralwidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 800, 25));
        menuBar->setStyleSheet(QString::fromUtf8("QMenuBar {\n"
"    background-color: grey;\n"
"}\n"
"QMenuBar::item {\n"
"    color:  #3CB371;\n"
"}\n"
""));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName("menuFile");
        menuFile->setStyleSheet(QString::fromUtf8(""));
        menuAbout = new QMenu(menuBar);
        menuAbout->setObjectName("menuAbout");
        menuAbout->setStyleSheet(QString::fromUtf8(""));
        MainWindow->setMenuBar(menuBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuAbout->menuAction());
        menuFile->addAction(actionSettings);
        menuFile->addAction(actionAdd_Game);
        menuFile->addAction(actionExit_Application);
        menuFile->addAction(fileActionStartRecording);
        menuFile->addAction(fileActionStopRecording);
        menuFile->addAction(actionRemove_Game);
        menuAbout->addAction(actionGitHub);
        menuAbout->addAction(actionInfo);
        menuAbout->addAction(actionKo_Fi);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        actionSettings->setText(QCoreApplication::translate("MainWindow", "Settings", nullptr));
        actionAdd_Game->setText(QCoreApplication::translate("MainWindow", "Add Game", nullptr));
        actionGitHub->setText(QCoreApplication::translate("MainWindow", "GitHub", nullptr));
        actionInfo->setText(QCoreApplication::translate("MainWindow", "About MineRecordEX", nullptr));
        actionExit_Application->setText(QCoreApplication::translate("MainWindow", "Exit Program", nullptr));
        fileActionStartRecording->setText(QCoreApplication::translate("MainWindow", "Start Recording", nullptr));
        fileActionStopRecording->setText(QCoreApplication::translate("MainWindow", "Stop Recording", nullptr));
        actionRemove_Game->setText(QCoreApplication::translate("MainWindow", "Remove Game", nullptr));
        actionKo_Fi->setText(QCoreApplication::translate("MainWindow", "Ko-Fi", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuAbout->setTitle(QCoreApplication::translate("MainWindow", "About", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
