TEMPLATE    = app
TARGET      = dqml
DESTDIR     = ../bin

CONFIG      -= app_bundle

QT          += quick

LIBS        += -L../lib -ldqmlmonitor -ldqmlserver
   
SOURCES     += \
    dqmllocalserver.cpp \
    dqmlmain.cpp \


HEADERS += \
    dqmllocalserver.h \
