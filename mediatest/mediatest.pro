TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
	main.c

HEADERS += \

LIBS += \
	-lortp \
    -lmediastreamer_base \
    -lmediastreamer_voip \
    -lpthread

