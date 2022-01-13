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

#include "writejob.h"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusUnixFileDescriptor>
#include <QProcess>
#include <QTextStream>
#include <QTimer>
#include <QtDBus>
#include <QtGlobal>

#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <tuple>
#include <utility>

#include <lzma.h>

typedef QHash<QString, QVariant> Properties;
typedef QHash<QString, Properties> InterfacesAndProperties;
typedef QHash<QDBusObjectPath, InterfacesAndProperties> DBusIntrospection;
Q_DECLARE_METATYPE(Properties)
Q_DECLARE_METATYPE(InterfacesAndProperties)
Q_DECLARE_METATYPE(DBusIntrospection)

// NOTE: aligned buffers are used for reading and
// writing to ensure optimal speed
class PageAlignedBuffer {
public:
    PageAlignedBuffer(const size_t page_count = 1024);
    ~PageAlignedBuffer();

    void *unaligned_buffer;
    void *buffer;
    size_t size;
};

WriteJob::WriteJob(const QString &what, const QString &where)
: QObject(nullptr)
, what(what)
, where(where) {
    qDBusRegisterMetaType<Properties>();
    qDBusRegisterMetaType<InterfacesAndProperties>();
    qDBusRegisterMetaType<DBusIntrospection>();
    connect(
        &watcher, &QFileSystemWatcher::fileChanged,
        this, &WriteJob::onFileChanged);
    QTimer::singleShot(0, this, SLOT(work()));
}

QDBusUnixFileDescriptor WriteJob::getDescriptor() {
    QDBusInterface device("org.freedesktop.UDisks2", where, "org.freedesktop.UDisks2.Block", QDBusConnection::systemBus(), this);
    QString drivePath = qvariant_cast<QDBusObjectPath>(device.property("Drive")).path();
    QDBusInterface manager("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", "org.freedesktop.DBus.ObjectManager", QDBusConnection::systemBus());
    QDBusMessage message = manager.call("GetManagedObjects");

    if (message.arguments().length() == 1) {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(message.arguments().first());
        DBusIntrospection objects;
        arg >> objects;
        for (const QDBusObjectPath &i : objects.keys()) {
            if (objects[i].contains("org.freedesktop.UDisks2.Filesystem")) {
                QString currentDrivePath = qvariant_cast<QDBusObjectPath>(objects[i]["org.freedesktop.UDisks2.Block"]["Drive"]).path();
                if (currentDrivePath == drivePath) {
                    QDBusInterface partition("org.freedesktop.UDisks2", i.path(), "org.freedesktop.UDisks2.Filesystem", QDBusConnection::systemBus());
                    message = partition.call("Unmount", Properties{{"force", true}});
                }
            }
        }
    } else {
        err << message.errorMessage();
        err.flush();
        qApp->exit(2);
        return QDBusUnixFileDescriptor(-1);
    }

    QDBusReply<QDBusUnixFileDescriptor> reply = device.callWithArgumentList(QDBus::Block, "OpenDevice", {"rw", Properties{{"flags", O_DIRECT | O_SYNC | O_CLOEXEC}, {"writable", true}}});
    QDBusUnixFileDescriptor fd = reply.value();

    if (!fd.isValid()) {
        err << reply.error().message();
        err.flush();
        qApp->exit(2);
        return QDBusUnixFileDescriptor(-1);
    }

    return fd;
}

bool WriteJob::write(int fd) {
    if (what.endsWith(".xz")) {
        return writeCompressed(fd);
    } else {
        return writePlain(fd);
    }
}

bool WriteJob::writeCompressed(int fd) {
    qint64 totalRead = 0;

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret;

    const PageAlignedBuffer inBuffer;
    const PageAlignedBuffer outBuffer;

    QFile file(what);
    const bool open_success = file.open(QIODevice::ReadOnly);
    if (!open_success) {
        err << tr("Source image is not readable") << what;
        err.flush();
        qApp->exit(2);
        return false;
    }

    ret = lzma_stream_decoder(&strm, MEDIAWRITER_LZMA_LIMIT, LZMA_CONCATENATED);
    if (ret != LZMA_OK) {
        err << tr("Failed to start decompressing.");
        return false;
    }

    strm.next_in = (uint8_t *) inBuffer.buffer;
    strm.avail_in = 0;
    strm.next_out = (uint8_t *) outBuffer.buffer;
    strm.avail_out = outBuffer.size;

    while (true) {
        if (strm.avail_in == 0) {
            qint64 len = file.read((char *) inBuffer.buffer, inBuffer.size);
            totalRead += len;

            strm.next_in = (uint8_t *) inBuffer.buffer;
            strm.avail_in = len;

            out << totalRead << "\n";
            out.flush();
        }

        ret = lzma_code(&strm, strm.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
        if (ret == LZMA_STREAM_END) {
            quint64 len = ::write(fd, outBuffer.buffer, outBuffer.size - strm.avail_out);
            if (len != outBuffer.size - strm.avail_out) {
                err << tr("Destination drive is not writable");
                qApp->exit(3);
                return false;
            }
            return true;
        }
        if (ret != LZMA_OK) {
            switch (ret) {
                case LZMA_MEM_ERROR:
                    err << tr("There is not enough memory to decompress the file.");
                    break;
                case LZMA_FORMAT_ERROR:
                case LZMA_DATA_ERROR:
                case LZMA_BUF_ERROR:
                    err << tr("The downloaded compressed file is corrupted.");
                    break;
                case LZMA_OPTIONS_ERROR:
                    err << tr("Unsupported compression options.");
                    break;
                default:
                    err << tr("Unknown decompression error.");
                    break;
            }
            qApp->exit(4);
            return false;
        }

        if (strm.avail_out == 0) {
            quint64 len = ::write(fd, outBuffer.buffer, outBuffer.size - strm.avail_out);
            if (len != outBuffer.size - strm.avail_out) {
                err << tr("Destination drive is not writable");
                qApp->exit(3);
                return false;
            }
            strm.next_out = (uint8_t *) outBuffer.buffer;
            strm.avail_out = outBuffer.size;
        }
    }
}

bool WriteJob::writePlain(int fd) {
    QFile inFile(what);
    const bool open_success = inFile.open(QIODevice::ReadOnly);
    if (!open_success) {
        err << tr("Source image is not readable") << what;
        err.flush();
        qApp->exit(2);
        return false;
    }

    const PageAlignedBuffer buffer;
    qint64 total = 0;

    while (!inFile.atEnd()) {
        qint64 len = inFile.read((char *) buffer.buffer, buffer.size);
        if (len < 0) {
            err << tr("Source image is not readable");
            err.flush();
            qApp->exit(3);
            return false;
        }
    try_again:
        qint64 written = ::write(fd, buffer.buffer, len);
        if (written != len) {
            if (written < 0) {
                if (errno == EIO) {
                    static int skip_once = 0;
                    if (!skip_once) {
                        skip_once = 1;
                        goto try_again;
                    }
                }
            }
            err << tr("Destination drive is not writable");
            err.flush();
            qApp->exit(3);
            return false;
        }
        total += len;
        out << total << '\n';
        out.flush();
    }

    inFile.close();
    sync();

    return true;
}

void WriteJob::work() {
    // have to keep the QDBus wrapper, otherwise the file gets closed
    fd = getDescriptor();
    if (fd.fileDescriptor() < 0) {
        return;
    }

    const bool delayed_write = [&]() {
        const QString part_path = what + ".part";

        return (QFile::exists(part_path) && !QFile::exists(what));
    }();

    if (delayed_write) {
        watcher.addPath(what + ".part");

        return;
    }

    // NOTE: let the app know that writing started
    out << "WRITE\n";
    out.flush();

    const bool write_success = write(fd.fileDescriptor());

    if (write_success) {
        out.flush();
        err << "DONE\n";
        qApp->exit(0);
    } else {
        qApp->exit(4);
    }
}

void WriteJob::onFileChanged(const QString &path) {
    const bool still_downloading = QFile::exists(path);
    if (still_downloading) {
        return;
    }

    const bool downloaded_file_exists = QFile::exists(what);
    if (!downloaded_file_exists) {
        qApp->exit(4);
        return;
    }

    work();
}

PageAlignedBuffer::PageAlignedBuffer(const size_t page_count) {
    static const size_t page_size = getpagesize();
    size = page_count * page_size;
    const size_t unaligned_size = size + page_size;
    unaligned_buffer = malloc(unaligned_size * sizeof(uint8_t));

    // NOTE: align() modifies space and ptr args to
    // return values for aligned buffer
    void *ptr_arg = unaligned_buffer;
    size_t space_arg = unaligned_size;

    buffer = std::align(page_size, size, ptr_arg, space_arg);
}

PageAlignedBuffer::~PageAlignedBuffer() {
    free(unaligned_buffer);
}
