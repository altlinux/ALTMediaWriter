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

#include "architecture.h"

#include <QObject>

const QList<Architecture> architecture_all = []() {
    QList<Architecture> out; 

    for (int i = 0; i < Architecture_COUNT; i++) {
        const Architecture architecture = (Architecture) i;
        out.append(architecture);
    }

    return out;
}();

QStringList architecture_strings(const Architecture architecture) {
    switch (architecture) {
        case Architecture_ALL: return QStringList();
        case Architecture_X86_64: return {"x86-64", "x86_64"};
        case Architecture_X86: return {"x86", "i386", "i586", "i686"};
        case Architecture_ARM: return {"armv7hl", "armhfp", "armh"};
        case Architecture_AARCH64: return {"aarch64"};
        case Architecture_MIPSEL: return {"mipsel"};
        case Architecture_RISCV64: return {"riscv", "riscv64"};
        case Architecture_E2K: return {"e2k"};
        case Architecture_PPC64LE: return {"ppc64le"};
        case Architecture_UNKNOWN: return QStringList();
        case Architecture_COUNT: return QStringList();
    }
    return QStringList();
}

QString architecture_name(const Architecture architecture) {
    switch (architecture) {
        case Architecture_ALL: return QObject::tr("All");
        case Architecture_X86_64: return QObject::tr("AMD 64bit");
        case Architecture_X86: return QObject::tr("Intel 32bit");
        case Architecture_ARM: return QObject::tr("ARM v7");
        case Architecture_AARCH64: return QObject::tr("AArch64");
        case Architecture_MIPSEL: return QObject::tr("MIPS");
        case Architecture_RISCV64: return QObject::tr("RiscV64");
        case Architecture_E2K: return QObject::tr("Elbrus");
        case Architecture_PPC64LE: return QObject::tr("PowerPC");
        case Architecture_UNKNOWN: return QObject::tr("Unknown");
        case Architecture_COUNT: return QString();
    }
    return QString();
}

Architecture architecture_from_string(const QString &string) {
    for (const Architecture architecture : architecture_all) {
        const QStringList strings = architecture_strings(architecture);

        if (strings.contains(string, Qt::CaseInsensitive)) {
            return architecture;
        }
    }
    return Architecture_UNKNOWN;
}

Architecture architecture_from_filename(const QString &filename) {
    for (const Architecture architecture : architecture_all) {
        const QStringList strings = architecture_strings(architecture);
        
        for (const QString &string : strings) {
            if (filename.contains(string, Qt::CaseInsensitive)) {
                return architecture;
            }
        }
    }
    return Architecture_UNKNOWN;
}
