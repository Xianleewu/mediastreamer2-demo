TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c

INCLUDEPATH += /usr/local/include \


LIBS += \
    -L/usr/local/lib \
    -lortp \
    -lmediastreamer_base \
    -lmediastreamer_voip \
    -lpthread

HEADERS +=
