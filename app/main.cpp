/*
 * ALT Media Writer
 * Copyright (C) 2016-2019 Martin Bříza <mbriza@redhat.com>
 * Copyright (C) 2020-2022 Dmitry Degtyarev <kevl@basealt.ru>
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
#include "units.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QTranslator>
#include <QtPlugin>

#ifdef __linux
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
#error "Minimum supported Qt version is 6.6.0"
#endif

#ifdef QT_STATIC
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuick_WindowPlugin);
Q_IMPORT_PLUGIN(QtQuickDialogsPlugin);
Q_IMPORT_PLUGIN(QtQuickDialogs2QuickImplPlugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin);
Q_IMPORT_PLUGIN(QmlFolderListModelPlugin);
#endif

void myMessageOutput(QtMsgType, const QMessageLogContext &, const QString &msg) {
    printf("%s\n", qPrintable(msg));
    fflush(stdout);
}

int main(int argc, char **argv) {

// Use software rendering on Windows because hardware
// rendering makes fuzzy unreadable fonts on Windows 7. Note
// that software rendering makes animations stuttery but
// this downside is not as bad as unreadable fonts.
#ifndef __linux
    if (qEnvironmentVariableIsEmpty("QMLSCENE_DEVICE")) {
        qputenv("QMLSCENE_DEVICE", "softwarecontext");
    }
#endif

    QApplication::setOrganizationDomain("basealt.ru");
    QApplication::setOrganizationName("BaseALT");
    QApplication::setApplicationName("ALTMediaWriter");
    // NOTE: don't set display name because it's
    // already displayed in main.qml's title. If
    // display name is set, then window is named "ALT
    // Media Writer - ALTMediaWriter" - no good.

    qInstallMessageHandler(myMessageOutput);
    QApplication app(argc, argv);

#ifdef __linux
    if (QGuiApplication::platformName() == QStringLiteral("xcb"))
    {
        if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP"))
            qputenv("QSG_RENDER_LOOP", "threaded");
    }
#endif

    qDebug() << "Application constructed";

    app.setWindowIcon(QIcon(":/mediawriter.svg"));

    QTranslator translator;
    translator.load(QLocale(QLocale().language(), QLocale().country()), QString(), QString(), ":/translations");
    app.installTranslator(&translator);

    qDebug() << "Injecting QML context properties";
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("drives", DriveManager::instance());
    engine.rootContext()->setContextProperty("releases", new ReleaseManager());
    engine.rootContext()->setContextProperty("mediawriterVersion", MEDIAWRITER_VERSION);
    engine.rootContext()->setContextProperty("units", Units::instance());

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
