TEMPLATE     = lib
TARGET       = dqmlserver
DESTDIR      = ../lib
CONFIG       += static

QT          += quick

SOURCES     += \
    dqmlglobal.cpp \
    dqmllocalserver.cpp \
    dqmlserver.cpp \


HEADERS     += \
    dqmlglobal.h \
    dqmllocalserver.h \
    dqmlserver.h \

