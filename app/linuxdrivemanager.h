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

#ifndef LINUXDRIVEMANAGER_H
#define LINUXDRIVEMANAGER_H

#include "drivemanager.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QProcess>

typedef QHash<QString, QVariantMap> InterfacesAndProperties;
typedef QHash<QDBusObjectPath, InterfacesAndProperties> DBusIntrospection;
Q_DECLARE_METATYPE(InterfacesAndProperties)
Q_DECLARE_METATYPE(DBusIntrospection)

class LinuxDriveProvider;
class LinuxDrive;
class Variant;

class LinuxDriveProvider : public DriveProvider {
    Q_OBJECT
public:
    LinuxDriveProvider(DriveManager *parent);

private slots:
    void delayedConstruct();
    void init(QDBusPendingCallWatcher *w);
    void onInterfacesAdded(const QDBusObjectPath &object_path, const InterfacesAndProperties &interfaces_and_properties);
    void onInterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);
    void onPropertiesChanged(const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties);

private:
    QDBusObjectPath handleObject(const QDBusObjectPath &path, const InterfacesAndProperties &interface);

private:
    QDBusInterface *m_objManager;
    QHash<QDBusObjectPath, LinuxDrive *> m_drives;
};

class LinuxDrive : public Drive {
    Q_OBJECT
    Q_PROPERTY(QString devicePath READ devicePath CONSTANT)
public:
    LinuxDrive(LinuxDriveProvider *parent, const QString &device, const QString &name, const uint64_t size, const bool isoLayout);
    ~LinuxDrive();

    Q_INVOKABLE virtual bool write(Variant *variant) override;
    Q_INVOKABLE virtual void cancel() override;
    Q_INVOKABLE virtual void restore() override;

    QString devicePath() const;

private slots:
    void onReadyRead();
    void onFinished(const int exitCode, const QProcess::ExitStatus status);
    void onRestoreFinished(const int exitCode, const QProcess::ExitStatus status);
    void onErrorOccurred(QProcess::ProcessError e);

private:
    QString m_device;

    QProcess *m_process;
};

#endif // LINUXDRIVEMANAGER_H
