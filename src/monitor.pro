TEMPLATE    = app
TARGET      = mortalqmlmonitor
DESTDIR     = ../bin

QT          = core network

CONFIG      -= app_bundle

SOURCES += \ 
     	filetracker.cpp \
        global.cpp \
        monitormain.cpp \

HEADERS += \
    filetracker.h \
    global.h
