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

#include "network.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

QNetworkAccessManager *network_access_manager = new QNetworkAccessManager();

NetworkReplyGroup::NetworkReplyGroup(const QList<QString> &url_list, QObject *parent)
: QObject(parent) {
    reply_list = [&]() {
        QHash<QString, QNetworkReply *> out;
        for (const QString &url : url_list) {
            QNetworkReply *reply = makeNetworkRequest(url, 5000);

            out[url] = reply;
        }

        return out;
    }();

    for (QNetworkReply *reply : reply_list) {
        connect(
            reply, &QNetworkReply::finished,
            this, &NetworkReplyGroup::on_reply_finished);
    }
}

NetworkReplyGroup::~NetworkReplyGroup() {
    // NOTE: network replies have to be deleted using
    // deleteLater(), deleting using normal delete
    // causes problems
    for (QNetworkReply *reply : reply_list) {
        reply->deleteLater();
    }
}

QHash<QString, QNetworkReply *> NetworkReplyGroup::get_reply_list() const {
    return reply_list;
}

// Emit finished() signal when all requests are
// finished
void NetworkReplyGroup::on_reply_finished() {
    const bool all_finished = [&]() {
        for (const QNetworkReply *reply : reply_list) {
            const bool finished = reply->isFinished();

            if (!finished) {
                return false;
            }
        }

        return true;
    }();

    if (all_finished) {
        emit finished();
    }
}

QNetworkReply *makeNetworkRequest(const QString &url, const int time_out_millis) {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply *reply = network_access_manager->get(request);

    // TODO: Qt 5.15 added QNetworkRequest::setTransferTimeout()
    // Abort download if it takes more than 5s
    if (time_out_millis > 0) {
        auto time_out_timer = new QTimer(reply);
        QObject::connect(
            time_out_timer, &QTimer::timeout,
            [reply]() {
                reply->abort();
            });
        time_out_timer->start(time_out_millis);
    }

    return reply;
}
