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

#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include <QList>
#include <QString>

enum Architecture {
    Architecture_ALL,
    Architecture_X86_64,
    Architecture_X86,
    Architecture_ARM,
    Architecture_AARCH64,
    Architecture_MIPSEL,
    Architecture_RISCV64,
    Architecture_E2K,
    Architecture_PPC64LE,
    Architecture_UNKNOWN,
    Architecture_COUNT,
};

extern const QList<Architecture> architecture_all;

QStringList architecture_strings(const Architecture architecture);
QString architecture_name(const Architecture architecture);
Architecture architecture_from_string(const QString &string);
Architecture architecture_from_filename(const QString &filename);

#endif // ARCHITECTURE_H
