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

#include "platform.h"

#include <QObject>

const QList<Platform> platform_all = []() {
    QList<Platform> out;

    for (int i = 0; i < Platform_COUNT; i++) {
        const Platform platform = (Platform) i;
        out.append(platform);
    }

    return out;
}();

QStringList platform_strings(const Platform platform) {
    switch (platform) {
        case Platform_ALL: return QStringList();
        case Platform_p10: return {"p10"};
        case Platform_p9: return {"p9"};
        case Platform_p8: return {"p8"};
        case Platform_UNKNOWN: return QStringList();
        case Platform_COUNT: return QStringList();
    }
    return QStringList();
}

QString platform_name(const Platform platform) {
    switch (platform) {
        case Platform_ALL: return QObject::tr("All");
        case Platform_p10: return QObject::tr("P10");
        case Platform_p9: return QObject::tr("P9");
        case Platform_p8: return QObject::tr("P8");
        case Platform_UNKNOWN: return QObject::tr("Unknown");
        case Platform_COUNT: return QString();
    }
    return QString();
}

Platform platform_from_string(const QString &string) {
    for (const Platform &platform : platform_all) {
        const QStringList strings = platform_strings(platform);

        if (strings.contains(string, Qt::CaseInsensitive)) {
            return platform;
        }
    }
    return Platform_UNKNOWN;
}
