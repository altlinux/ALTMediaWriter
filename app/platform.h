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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <QList>
#include <QString>

enum Platform {
    Platform_ALL,
    Platform_p10,
    Platform_p9,
    Platform_p8,
    Platform_UNKNOWN,
    Platform_COUNT,
};

extern const QList<Platform> platform_all;

QStringList platform_strings(const Platform platform);
QString platform_name(const Platform platform);
Platform platform_from_string(const QString &string);

#endif // PLATFORM_H