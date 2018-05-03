#-------------------------------------------------
#
# Project created by QtCreator 2018-05-02T23:36:43
#
#-------------------------------------------------

QT       += core gui
QT      +=network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyChat
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui

RESOURCES += \
    image/image.qrc
