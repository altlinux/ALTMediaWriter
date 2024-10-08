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

#include <QCoreApplication>
#include <QTextStream>
#include <QTranslator>

#include "restorejob.h"
#include "writejob.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QTranslator translator;
    translator.load(QLocale(), QString(), QString(), ":/translations");
    app.installTranslator(&translator);

    if (app.arguments().count() == 3 && app.arguments()[1] == "restore") {
        new RestoreJob(app.arguments()[2]);
    } else if (app.arguments().count() == 5 && app.arguments()[1] == "write") {
        new WriteJob(app.arguments()[2], app.arguments()[3], app.arguments()[4]);
    } else {
        QTextStream err(stderr);
        err << "Helper: Wrong arguments entered";
        return 1;
    }
    return app.exec();
}
