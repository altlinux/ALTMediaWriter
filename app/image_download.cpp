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

#include "image_download.h"
#include "network.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTimer>

ImageDownload::ImageDownload(const QUrl &url_arg, const QString &filePath_arg, const QString &md5sum_arg)
: QObject()
, hash(QCryptographicHash::Md5) {
    url = url_arg;
    filePath = filePath_arg;
    md5sum = md5sum_arg;
    file = nullptr;
    startingImageDownload = false;
    wasCancelled = false;

    qDebug() << this->metaObject()->className() << "created for" << url;

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    const QString tempFilePath = filePath + ".part";
    file = new QFile(tempFilePath, this);
    file->open(QIODevice::WriteOnly | QIODevice::Append);

    startImageDownload();
}

ImageDownload::Result ImageDownload::result() const {
    return m_result;
}

QString ImageDownload::errorString() const {
    return m_errorString;
}

void ImageDownload::cancel() {
    if (wasCancelled) {
        return;
    }

    qDebug() << this->metaObject()->className() << "Cancelling download";

    wasCancelled = true;
    emit cancelled();

    finish(ImageDownload::Cancelled);
}

void ImageDownload::onImageDownloadReadyRead() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (startingImageDownload) {
        qDebug() << "Request started successfully";
        startingImageDownload = false;

        const QVariant remainingSize = reply->header(QNetworkRequest::ContentLengthHeader);
        if (remainingSize.isValid()) {
            const qint64 totalSize = file->size() + remainingSize.toULongLong();

            emit progressMaxChanged(totalSize);
        }

        emit started();
    }

    const QByteArray data = reply->readAll();
    if (reply->error() == QNetworkReply::NoError && data.size() > 0) {
        if (reply->header(QNetworkRequest::ContentLengthHeader).isValid()) {
            ;
        }

        const qint64 writeSize = file->write(data);
        const bool writeSuccess = (writeSize != -1);
        if (writeSuccess) {
            emit progress(file->size());
        } else {
            QStorageInfo storage(file->fileName());
            const QString errorString = [storage]() {
                if (storage.bytesAvailable() < 5L * 1024L * 1024L) {
                    return tr("You ran out of space in your Downloads folder.");
                } else {
                    return tr("The downloaded file is not writable.");
                }
            }();

            finish(ImageDownload::DiskError, errorString);
        }
    }
}

void ImageDownload::onImageDownloadFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (wasCancelled) {
        return;
    } else if (reply->error() == QNetworkReply::NoError) {
        qDebug() << this->metaObject()->className() << "Finished successfully";

        if (md5sum.isEmpty()) {
            // If md5sum doesn't exist, be lenient and
            // don't treat this as a failed check.
            // Instead, skip the check.
            qDebug() << this->metaObject()->className() << "No md5sum found, so skipping md5 check";

            rename_to_final_name();
        } else {
            file->close();
            const bool open_success = file->open(QIODevice::ReadOnly);
            if (open_success) {
                emit startedMd5Check();
                QTimer::singleShot(0, this, &ImageDownload::computeMd5);
            } else {
                qDebug() << this->metaObject()->className() << "Failed to open file for md5 check";

                finish(ImageDownload::Md5CheckFail);
            }
        }
    } else {
        qDebug() << "Download was interrupted by an error:" << reply->errorString();
        qDebug() << "Attempting to resume";

        emit interrupted();

        QTimer::singleShot(1000, this,
            [this]() {
                startImageDownload();
            });
    }
}

void ImageDownload::computeMd5() {
    if (wasCancelled) {
        return;
    }

    const QByteArray bytes = file->read(64L * 1024L);
    const bool read_success = (bytes.size() > 0);

    if (read_success) {
        hash.addData(bytes);
        emit progress(file->pos());

        if (file->atEnd()) {
            const QByteArray sum_bytes = hash.result().toHex();
            const QString computedMd5 = QString(sum_bytes);

            const bool checkPassed = (computedMd5 == md5sum);

            if (checkPassed) {
                qDebug() << "MD5 check passed";

                rename_to_final_name();
            } else {
                qDebug() << "MD5 mismatch";
                qDebug() << "sum should be =" << md5sum;
                qDebug() << "computed sum  =" << computedMd5;

                finish(ImageDownload::Md5CheckFail);
            }
        } else {
            QTimer::singleShot(0, this, &ImageDownload::computeMd5);
        }
    } else {
        finish(ImageDownload::Md5CheckFail, tr("Failed to read from file while verifying"));
    }
}

void ImageDownload::startImageDownload() {
    qDebug() << this->metaObject()->className() << "startImageDownload()";

    startingImageDownload = true;

    QNetworkRequest request;
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setUrl(url);
    request.setRawHeader("Range", QString("bytes=%1-").arg(file->size()).toLocal8Bit());

    QNetworkReply *reply = network_access_manager->get(request);
    // NOTE: 64MB buffer in case the user is on a very fast network
    reply->setReadBufferSize(64L * 1024L * 1024L);

    connect(
        reply, &QNetworkReply::readyRead,
        this, &ImageDownload::onImageDownloadReadyRead);
    connect(
        reply, &QNetworkReply::finished,
        this, &ImageDownload::onImageDownloadFinished);
    connect(
        this, &ImageDownload::cancelled,
        reply, &QNetworkReply::abort);
}

void ImageDownload::rename_to_final_name() {
    qDebug() << this->metaObject()->className() << "Renaming to final filename";

    const bool rename_success = file->rename(filePath);

    if (rename_success) {
        finish(ImageDownload::Success);
    } else {
        finish(ImageDownload::DiskError, tr("Unable to rename the temporary file."));
    }
}

void ImageDownload::finish(const Result result_arg, const QString &errorString_arg) {
    m_result = result_arg;
    m_errorString = errorString_arg;

    qDebug() << "Image download finished";
    if (!m_errorString.isEmpty()) {
        qDebug() << "Error string:" << m_errorString;
    }

    if (m_result == ImageDownload::Success || m_result == ImageDownload::Cancelled) {
        file->close();
    } else {
        file->remove();
    }

    emit finished();

    deleteLater();
}
