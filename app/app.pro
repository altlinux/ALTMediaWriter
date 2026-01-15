TEMPLATE = app

include($$top_srcdir/deployment.pri)

TARGET = $$MEDIAWRITER_NAME

QT += qml quick quickcontrols2 widgets network

LIBS += -lisomd5
linux {
    LIBS += -lyaml-cpp
}
windows {
    # NOTE: "-lyaml-cpp" ignores static linking option and links to dynamic "yaml.cpp.dll.a", so have to manually link to static file
    LIBS += -l:libyaml-cpp.a
}

CONFIG += c++17

HEADERS += \
    drivemanager.h \
    releasemanager.h \
    network.h \
    notifications.h \
    image_download.h \
    progress.h \
    file_type.h \
    architecture.h \
    platform.h \
    release.h \
    release_model.h \
    units.h \
    variant.h

SOURCES += main.cpp \
    drivemanager.cpp \
    releasemanager.cpp \
    network.cpp \
    notifications.cpp \
    image_download.cpp \
    progress.cpp \
    file_type.cpp \
    architecture.cpp \
    platform.cpp \
    release.cpp \
    release_model.cpp \
    units.cpp \
    variant.cpp

RESOURCES += qml.qrc \
    assets.qrc \
    ../translations/translations.qrc

lupdate_only {
    SOURCES += $$PWD/*.qml \
        $$PWD/complex/*.qml \
        $$PWD/dialogs/*.qml \
        $$PWD/simple/*.qml \
        $$PWD/views/*.qml \
        linuxdrivemanager.cpp \
        windrivemanager.cpp
    HEADERS += linuxdrivemanager.h \
        windrivemanager.h
}

target.path = $$BINDIR
INSTALLS += target

linux {
    QT += dbus

    HEADERS += linuxdrivemanager.h
    SOURCES += linuxdrivemanager.cpp

    app_icon.path = "$$DATADIR/icons/hicolor/scalable/apps/"
    app_icon.files = assets/icon/apps/mediawriter.svg

    symbolic_icons.path = "$$DATADIR/icons/hicolor/symbolic/apps/"
    symbolic_icons.files = assets/icon/apps/*-symbolic.svg

    desktopfile.path = "$$DATADIR/applications/"
    desktopfile.files = "$$top_srcdir/dist/linux/mediawriter.desktop"

    appdatafile.path = "$$DATADIR/appdata/"
    appdatafile.files = "$$top_srcdir/dist/linux/mediawriter.appdata.xml"

    INSTALLS += app_icon symbolic_icons desktopfile appdatafile
}
win32 {
    HEADERS += windrivemanager.h
    SOURCES += windrivemanager.cpp
    RESOURCES += windowsicon.qrc

    LIBS += -ldbghelp

    # Until I find out how (or if it's even possible at all) to run a privileged process from an unprivileged one, the main binary will be privileged too
    DISTFILES += windows.manifest
    QMAKE_MANIFEST = $${PWD}/windows.manifest

    RC_ICONS = assets/icon/mediawriter.ico
    static {
        QTPLUGIN.platforms = qwindows
    }
}
