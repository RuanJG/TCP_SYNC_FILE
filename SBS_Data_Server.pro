#-------------------------------------------------
#
# Project created by QtCreator 2018-10-12T09:07:40
#
#-------------------------------------------------

QT       += core gui network printsupport axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SBSDataServer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tcpserverworker.cpp \
    datastorer.cpp \
    excelengine.cpp \
    serialcoder.cpp

HEADERS  += mainwindow.h \
    tcpserverworker.h \
    datastorer.h \
    excelengine.h \
    serialcoder.h

FORMS    += mainwindow.ui

RC_FILE += logo.rc

DISTFILES += \
    logo.ico \
    logo.rc
