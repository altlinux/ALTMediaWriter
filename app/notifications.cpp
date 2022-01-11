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

#include "notifications.h"

#include <QDebug>
#include <QSystemTrayIcon>

#ifdef __linux

# include <QDBusInterface>

void Notifications::notify(const QString &title, const QString &body) {
    QDBusInterface notifications("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", QDBusConnection::sessionBus());
    const QDBusMessage reply = notifications.call("Notify", "ALT Media Writer", 0U, "mediawriter", title, body, QStringList{}, QVariantMap{}, -1);
    if (reply.type() == QDBusMessage::ErrorMessage)
        qDebug() << "Couldn't send a notification:" << reply.errorName() << "-" << reply.errorMessage();
}

#endif // __linux

#ifdef _WIN32

void Notifications::notify(const QString &title, const QString &body) {
    static QSystemTrayIcon *icon = new QSystemTrayIcon(QIcon(":/icon.ico"));
    if (!icon->isVisible())
        icon->show();
    icon->showMessage(title, body);
}

#endif // _WIN32


