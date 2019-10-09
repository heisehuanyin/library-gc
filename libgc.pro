TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ../ecosystem/excstream.cpp \
        gcobject.cpp \
        main.cpp

HEADERS += \
    ../ecosystem/excstream.h \
    ../ecosystem/sync.h \
    gcobject.h
