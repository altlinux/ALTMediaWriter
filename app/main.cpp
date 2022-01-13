/*
 * ALT Media Writer
 * Copyright (C) 2016-2019 Martin Bříza <mbriza@redhat.com>
 * Copyright (C) 2020 Dmitry Degtyarev <kevl@basealt.ru>
 *
 * ALT Media Writer is a fork of Fedora Media Writer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "drivemanager.h"
#include "progress.h"
#include "release.h"
#include "release_model.h"
#include "releasemanager.h"
#include "variant.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QTranslator>
#include <QtPlugin>

#ifdef __linux
#include <QX11Info>
#endif

#if QT_VERSION < 0x050300
#error "Minimum supported Qt version is 5.3.0"
#endif

#ifdef QT_STATIC
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuick2WindowPlugin);
Q_IMPORT_PLUGIN(QtQuick2DialogsPlugin);
Q_IMPORT_PLUGIN(QtQuick2DialogsPrivatePlugin);
Q_IMPORT_PLUGIN(QtQuickControls1Plugin);
Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin);
Q_IMPORT_PLUGIN(QmlFolderListModelPlugin);
Q_IMPORT_PLUGIN(QmlSettingsPlugin);
#endif

void myMessageOutput(QtMsgType, const QMessageLogContext &, const QString &msg) {
    printf("%s\n", qPrintable(msg));
    fflush(stdout);
}

int main(int argc, char **argv) {
#ifdef __linux
    if (QX11Info::isPlatformX11()) {
        if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP"))
            qputenv("QSG_RENDER_LOOP", "threaded");
        qputenv("GDK_BACKEND", "x11");
    }
#endif

    QApplication::setOrganizationDomain("altlinux.org");
    QApplication::setOrganizationName("basealt.ru");
    QApplication::setApplicationName("MediaWriter");

#ifdef __linux
    // qt x11 scaling is broken
    if (QX11Info::isPlatformX11()) {
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }
#else
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    qInstallMessageHandler(myMessageOutput);

    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);

    qDebug() << "Application constructed";

    QTranslator translator;
    translator.load(QLocale(QLocale().language(), QLocale().country()), QString(), QString(), ":/translations");
    app.installTranslator(&translator);

    qDebug() << "Injecting QML context properties";
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("drives", DriveManager::instance());
    engine.rootContext()->setContextProperty("releases", new ReleaseManager());
    engine.rootContext()->setContextProperty("mediawriterVersion", MEDIAWRITER_VERSION);

    qmlRegisterUncreatableType<ReleaseFilterModel>("MediaWriter", 1, 0, "ReleaseFilterModel", "");
    qmlRegisterUncreatableType<Release>("MediaWriter", 1, 0, "Release", "");
    qmlRegisterUncreatableType<Variant>("MediaWriter", 1, 0, "Variant", "");
    qmlRegisterUncreatableType<Progress>("MediaWriter", 1, 0, "Progress", "");
    qmlRegisterUncreatableType<Drive>("MediaWriter", 1, 0, "Drive", "");

    qDebug() << "Loading the QML source code";
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    qDebug() << "Starting the application";
    int status = app.exec();
    qDebug() << "Quitting with status" << status;

    return status;
}
