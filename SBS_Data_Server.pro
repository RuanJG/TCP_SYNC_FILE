#-------------------------------------------------
#
# Project created by QtCreator 2018-10-12T09:07:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SBS_Data_Server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tcpserverworker.cpp

HEADERS  += mainwindow.h \
    tcpserverworker.h

FORMS    += mainwindow.ui
