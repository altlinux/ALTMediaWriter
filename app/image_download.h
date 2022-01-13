/*
 * Fedora Media Writer
 * Copyright (C) 2017 Martin Bříza <mbriza@redhat.com>
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

#ifndef IMAGE_DOWNLOAD_H
#define IMAGE_DOWNLOAD_H

#include <QCryptographicHash>
#include <QObject>
#include <QUrl>

/**
 * Downloads an image using QNetwork and writes downloaded
 * image to disk in parallel. The default download directory
 * is used. While the image file is partially downloaded, it
 * is suffixed with ".part". This suffix is removed when the
 * image download completes. When the download is finished,
 * an attempt to check md5 is made. Md5 sum is downloaded
 * from the MD5SUM file which should be located next to the
 * image file. If md5sum download fails due to error or
 * MD5SUM file not being present, the check is skipped. If
 * the download is interrupted by an error or time out,
 * periodic attempts to resume are made. If the download
 * finishes unsuccessfully, partially downloaded image is
 * deleted. Image download schedules itself for deletion
 * when it finishes.
 */

class QFile;

class ImageDownload final : public QObject {
    Q_OBJECT

public:
    enum Result {
        Success,
        DiskError,
        Md5CheckFail,
        Cancelled
    };

    ImageDownload(const QUrl &url_arg, const QString &filePath_arg);
    Result result() const;
    QString errorString() const;

signals:
    // Emitted when download sucessfuly starts or resumes after being
    // interrupted.
    void started();

    // Emitted when the download is interrupted by an error or time out.
    // Note that download is not finished at this point, attempts to
    // resume are made periodically.
    void interrupted();

    // An md5 check for the downloaded image started. Note that md5 checks happen only if a valid md5 sum could be downloaded for this. Otherwise this check is skipped.
    void startedMd5Check();

    // Emitted when the download finishes
    void finished();

    void cancelled();

    //
    void progress(const qint64 value);
    void progressMaxChanged(const qint64 value);

public slots:
    void cancel();

private slots:
    void onImageDownloadReadyRead();
    void onImageDownloadFinished();
    void onMd5DownloadFinished();
    void computeMd5();

private:
    Result m_result;
    QString m_errorString;

    const QUrl url;
    const QString filePath;
    QFile *file = nullptr;
    bool startingImageDownload = false;
    bool wasCancelled = false;

    QCryptographicHash hash;
    QString md5;

    QString getFilePath() const;
    void startImageDownload();
    void checkMd5(const QString &computedMd5);
    void finish(const Result result_arg, const QString &errorString_arg = QString());
};

#endif // IMAGE_DOWNLOAD_H
