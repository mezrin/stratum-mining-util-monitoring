QT += core network

QT -= gui

TARGET = stratumon

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    alogger.cpp \
    amonitor.cpp \
    astratummonitor.cpp

HEADERS += \
    alogger.h \
    asingleton.h \
    amonitor.h \
    astratummonitor.h

OTHER_FILES += \
	README.md
