TEMPLATE    = lib
TARGET      = dqmlmonitor
DESTDIR     = ../lib

QT          = core network

CONFIG      -= app_bundle
CONFIG      += static

SOURCES += \ 
        dqmlfiletracker.cpp \
        dqmlglobal.cpp \
        dqmlmonitor.cpp \

HEADERS += \
        dqmlfiletracker.h \
        dqmlglobal.h \
        dqmlmonitor.h \
