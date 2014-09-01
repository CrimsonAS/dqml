TEMPLATE    = app
TARGET      = dqml
DESTDIR     = ../bin

CONFIG      -= app_bundle

QT          += quick

LIBS        += -L../lib -ldqmlserver
   
SOURCES     += \
    dqmlfiletracker.cpp \
    dqmlglobal.cpp \
    dqmlmain.cpp \
    dqmlmonitor.cpp \

HEADERS += \
    dqmlfiletracker.h \
    dqmlglobal.h \
    dqmlmonitor.h \
