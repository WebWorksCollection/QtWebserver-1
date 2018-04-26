QT = core network

CONFIG += c++14
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x060000

HEADERS += demohttpserver.h \
    randomdevice.h

SOURCES += main.cpp \
           demohttpserver.cpp \
    randomdevice.cpp

include(QtWebserver.pri)
