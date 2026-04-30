TEMPLATE = app

include($$top_srcdir/deployment.pri)

QT += core network

LIBS += -lisomd5 -llzma
QMAKE_LIBS += -lfreetype -lharfbuzz -lgraphite2 -lbz2  -lrpcrt4  -lpng16 -lz -lglib-2.0

CONFIG += c++17
CONFIG += console

TARGET = helper

target.path = $$LIBEXECDIR
INSTALLS += target

SOURCES = main.cpp \
    writejob.cpp \
    restorejob.cpp

HEADERS += \
    writejob.h \
    restorejob.h

RESOURCES += ../../translations/translations.qrc

DISTFILES += \
    helper.exe.manifest

QMAKE_MANIFEST = $${PWD}/helper.exe.manifest
