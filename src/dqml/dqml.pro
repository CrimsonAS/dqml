TEMPLATE    = lib
TARGET      = dqml

QT          += quick

load(qt_module)


CONFIG      -= create_cmake

SOURCES += \
        dqmlfiletracker.cpp \
        dqmlglobal.cpp \
        dqmllocalserver.cpp \
        dqmlmonitor.cpp \
        dqmlserver.cpp \

HEADERS += \
        dqmlfiletracker.h \
        dqmlglobal.h \
        dqmllocalserver.h \
        dqmlmonitor.h \
        dqmlserver.h \

DEFINES += DQML_BUILD_LIB=1
