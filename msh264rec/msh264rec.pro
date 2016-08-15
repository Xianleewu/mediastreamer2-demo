#-------------------------------------------------
#
# Project created by QtCreator 2016-08-15T09:44:22
#
#-------------------------------------------------

QT       -= core gui

TARGET = msh264rec
TEMPLATE = lib

DEFINES += MSH264REC_LIBRARY

SOURCES += msh264rec.c

HEADERS += msh264rec.h

unix {
    LIBS += \
        -L/usr/local/lib \
        -lortp \
        -lmediastreamer_base \
        -lmediastreamer_voip

    target.path = /usr/local/lib/mediastreamer/plugins
    INSTALLS += target
}
