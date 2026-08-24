/*
 * Copyright (C) 2001-2013 Red Hat, Inc.
 *
 * Michael Fulbright <msf@redhat.com>
 * Dustin Kirkland  <dustin.dirkland@gmail.com>
 *      Added support for checkpoint fragment sums;
 *      Exits media check as soon as bad fragment md5sum'ed
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>

#include <QCryptographicHash>
#include <QByteArrayView>

#include "libcheckisomd5.h"

#ifdef __APPLE__
#define lseek64 lseek
#endif

#ifdef _WIN32
size_t getpagesize () {
    return 2048; // not really necessary for Windows
}
#endif

#define BUFSIZE 32768
#define SIZE_OFFSET 84

#define MAX(x, y)  ((x > y) ? x : y)
#define MIN(x, y)  ((x < y) ? x : y)

static int checkmd5sum(int fd, const char *mediasum, checkCallback cb, void *cbdata, long long size) {
    // Md5 is empty, therefore md5 check not needed
    if (mediasum[0] == '\0') {
        return ISOMD5SUM_CHECK_PASSED;
    }
    
    int pagesize = getpagesize();
    unsigned char *buf_unaligned = (unsigned char *) malloc((BUFSIZE + pagesize) * sizeof(unsigned char));
    unsigned char *buf = (buf_unaligned + (pagesize - ((uintptr_t) buf_unaligned % pagesize)));

    // Rewind
    long long offset = lseek64(fd, 0LL, SEEK_SET);

    // Compute md5
    QCryptographicHash hash(QCryptographicHash::Md5);

    if (cb) {
        cb(cbdata, 0, size);
    }

    while (offset < size) {
        ssize_t nattempt = MIN(size - offset, BUFSIZE);

        ssize_t nread = read(fd, buf, nattempt);
        if (nread <= 0)
            break;

        if (nread > nattempt) {
            nread = nattempt;
            lseek64(fd, offset + nread, SEEK_SET);
        }

        hash.addData(QByteArrayView(
            reinterpret_cast<const char *>(buf),
            static_cast<qsizetype>(nread)));

        offset = offset + nread;
        if (cb && offset / nread % 256 == 0) {
            if (cb(cbdata, offset, size)) {
                free(buf_unaligned);
                return ISOMD5SUM_CHECK_ABORTED;
            }
        }
    }

    if (cb) {
        cb(cbdata, size, size);
    }

    free(buf_unaligned);

    const QByteArray computedsum_bytes = hash.result().toHex();
    const char *computed_sum = computedsum_bytes.constData();

    const bool sums_match = (memcmp(computed_sum, mediasum, computedsum_bytes.size()) == 0);

    if (sums_match) {
        return ISOMD5SUM_CHECK_PASSED;
    } else {
        return ISOMD5SUM_CHECK_FAILED;
    }
}

int mediaCheckFile(const char *file, const char *md5,checkCallback cb, void *cbdata) {
    int fd;

#ifdef _WIN32
    fd = open(file, O_RDONLY | O_BINARY);
#else
    fd = open(file, O_RDONLY);
#endif

    if (fd < 0) {
        return ISOMD5SUM_FILE_NOT_FOUND;
    }

    // Calculate file size
    long long size = lseek64(fd, 0L, SEEK_END);

    int rc = checkmd5sum(fd, md5, cb, cbdata, size);

    close(fd);

    return rc;
}

int mediaCheckFD(int fd, const char *md5, checkCallback cb, void *cbdata) {
    if (fd < 0) {
        return ISOMD5SUM_FILE_NOT_FOUND;
    }

    // NOTE: files that are FD(written to drive) are implicitly always iso's 
    // Get size
    int pagesize = getpagesize();
    unsigned char *buf_unaligned = (unsigned char *) malloc((BUFSIZE + pagesize) * sizeof(unsigned char));
    unsigned char *buf = (buf_unaligned + (pagesize - ((uintptr_t) buf_unaligned % pagesize)));
    if (lseek64(fd, (16LL * 2048LL), SEEK_SET) == -1) {
        free(buf_unaligned);
        return ISOMD5SUM_CHECK_NOT_FOUND;
    }

    long long offset = (16LL * 2048LL);
    for (;1;) {
        if (read(fd, buf, 2048) <= 0) {
            free(buf_unaligned);
            return ISOMD5SUM_CHECK_NOT_FOUND;
        }

        if (buf[0] == 1) {
        /* found primary volume descriptor */
            break;
        } else if (buf[0] == 255) {
        /* hit end and didn't find primary volume descriptor */
            free(buf_unaligned);
            return ISOMD5SUM_CHECK_NOT_FOUND;
        }
        offset += 2048LL;
    }

    // Get size from pvd
    long long size = (buf[SIZE_OFFSET] * 0x1000000 + buf[SIZE_OFFSET + 1] * 0x10000 + buf[SIZE_OFFSET + 2] * 0x100 + buf[SIZE_OFFSET + 3]) * 2048LL;

    free(buf_unaligned);

    int rc = checkmd5sum(fd, md5, cb, cbdata, size);

    return rc;
}
