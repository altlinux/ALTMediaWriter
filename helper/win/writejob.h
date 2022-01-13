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

#ifndef WRITEJOB_H
#define WRITEJOB_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QTextStream>

#include <windows.h>

#ifndef MEDIAWRITER_LZMA_LIMIT
// 256MB memory limit for the decompressor
#define MEDIAWRITER_LZMA_LIMIT (1024 * 1024 * 256)
#endif

class WriteJob : public QObject {
    Q_OBJECT
public:
    explicit WriteJob(const QString &what, const QString &where);

private:
    HANDLE openDrive(int physicalDriveNumber);
    bool lockDrive(HANDLE drive);
    bool removeMountPoints(uint diskNumber);
    // bool dismountDrive(HANDLE drive, int diskNumber);
    bool cleanDrive(uint diskNumber);

    bool writeBlock(HANDLE drive, OVERLAPPED *overlap, char *data, uint size);

    void unlockDrive(HANDLE drive);

private slots:
    void work();

    bool write();
    bool writeCompressed(HANDLE drive);
    bool writePlain(HANDLE drive);
    void onFileChanged(const QString &path);

private:
    QString what;
    uint where;

    QTextStream out{stdout};
    QTextStream err{stderr};
    QFileSystemWatcher watcher;

    const int BLOCK_SIZE{512 * 128};
};

#endif // WRITEJOB_H
