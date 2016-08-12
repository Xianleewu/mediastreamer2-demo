TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c

INCLUDEPATH += /usr/local/mediastreamer2/include \


LIBS += \
    -L/usr/local/lib \
    -lortp \
    -lmediastreamer_base \
    -lmediastreamer_voip \
    -lpthread
