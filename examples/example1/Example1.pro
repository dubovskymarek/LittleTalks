#CONFIG += c++11

include(../LittleTalks.pri)

TARGET = example1

TARGET = example1
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.c

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ../../LittleTalks
