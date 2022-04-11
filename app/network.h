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

#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QHash>

class QNetworkAccessManager;
class QNetworkReply;

extern QNetworkAccessManager *network_access_manager;

class NetworkReplyGroup final : public QObject {
    Q_OBJECT

public:
    NetworkReplyGroup(const QList<QString> &url_list, QObject *parent);
    ~NetworkReplyGroup();

    QHash<QString, QNetworkReply *> get_reply_list() const;

signals:
    void finished();

private:
    QHash<QString, QNetworkReply *> reply_list;

    void on_reply_finished();
};

QNetworkReply *makeNetworkRequest(const QString &url, const int time_out_millis = 0);

#endif // NETWORK_H
