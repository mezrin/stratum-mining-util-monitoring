QT += core network

QT -= gui

TARGET = stratumon

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
	adaemon.cpp

HEADERS += \
	adaemon.h

OTHER_FILES += \
	README.md
