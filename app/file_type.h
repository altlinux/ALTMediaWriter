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

#ifndef FILE_TYPE_H
#define FILE_TYPE_H

#include <QList>
#include <QString>

enum FileType {
    FileType_ISO,
    FileType_TAR,
    FileType_TAR_GZ,
    FileType_TAR_XZ,
    FileType_IMG,
    FileType_IMG_GZ,
    FileType_IMG_XZ,
    FileType_RECOVERY_TAR,
    FileType_UNKNOWN,
    FileType_COUNT,
};

extern const QList<FileType> file_type_all;

QStringList file_type_strings(const FileType file_type);
QString file_type_name(const FileType file_type);
FileType file_type_from_filename(const QString &filename);
bool file_type_can_write(const FileType file_type);

#endif // FILE_TYPE_H
