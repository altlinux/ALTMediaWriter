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

#include <QObject>
#include <QTextStream>
#include <QProcess>
#include <QFile>
#include <QDBusUnixFileDescriptor>
#include <QFileSystemWatcher>

#include <unistd.h>

#include <memory>
#include <tuple>
#include <utility>

#ifndef MEDIAWRITER_LZMA_LIMIT
// 256MB memory limit for the decompressor
# define MEDIAWRITER_LZMA_LIMIT (1024*1024*256)
#endif

class WriteJob : public QObject
{
    Q_OBJECT
public:
    explicit WriteJob(const QString &what, const QString &where);

    QDBusUnixFileDescriptor getDescriptor();
    bool write(int fd);
    bool writeCompressed(int fd);
    bool writePlain(int fd);
public slots:
    void work();
private slots:
    void onFileChanged(const QString &path);
private:
    QString what;
    QString where;
    QTextStream out { stdout };
    QTextStream err { stderr };
    QDBusUnixFileDescriptor fd { -1 };
    QFileSystemWatcher watcher;
};

#endif // WRITEJOB_H
