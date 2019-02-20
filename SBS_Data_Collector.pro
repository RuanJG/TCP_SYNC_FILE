#-------------------------------------------------
#
# Project created by QtCreator 2018-10-12T08:58:38
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SBSGapTest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RC_FILE += logo.rc

DISTFILES += \
    logo.rc \
    logo.ico
