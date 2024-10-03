/*
 * ALT Media Writer
 * Copyright (C) 2016-2019 Martin Bříza <mbriza@redhat.com>
 * Copyright (C) 2020-2022 Dmitry Degtyarev <kevl@basealt.ru>
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

#include "linuxdrivemanager.h"
#include "progress.h"
#include "variant.h"

#include <QDBusArgument>
#include <QtDBus/QtDBus>

#include "notifications.h"

LinuxDriveProvider::LinuxDriveProvider(DriveManager *parent)
: DriveProvider(parent) {
    m_objManager = nullptr;

    qDebug() << this->metaObject()->className() << "construction";
    qDBusRegisterMetaType<InterfacesAndProperties>();
    qDBusRegisterMetaType<DBusIntrospection>();

    m_initialized = false;

    QTimer::singleShot(0, this, SLOT(delayedConstruct()));
}

void LinuxDriveProvider::delayedConstruct() {
    m_objManager = new QDBusInterface("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", "org.freedesktop.DBus.ObjectManager", QDBusConnection::systemBus());

    qDebug() << this->metaObject()->className() << "Calling GetManagedObjects over DBus";
    QDBusPendingCall pcall = m_objManager->asyncCall("GetManagedObjects");
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(pcall, this);

    connect(w, &QDBusPendingCallWatcher::finished, this, &LinuxDriveProvider::init);

    QDBusConnection::systemBus().connect("org.freedesktop.UDisks2", 0, "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    QDBusConnection::systemBus().connect("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", "org.freedesktop.DBus.ObjectManager", "InterfacesAdded", this, SLOT(onInterfacesAdded(QDBusObjectPath, InterfacesAndProperties)));
    QDBusConnection::systemBus().connect("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved", this, SLOT(onInterfacesRemoved(QDBusObjectPath, QStringList)));
}

QDBusObjectPath LinuxDriveProvider::handleObject(const QDBusObjectPath &object_path, const InterfacesAndProperties &interfaces_and_properties) {
    QRegExp numberRE("[0-9]$");
    QRegExp mmcRE("[0-9]p[0-9]$");
    QDBusObjectPath driveId = qvariant_cast<QDBusObjectPath>(interfaces_and_properties["org.freedesktop.UDisks2.Block"]["Drive"]);

    QDBusInterface driveInterface("org.freedesktop.UDisks2", driveId.path(), "org.freedesktop.UDisks2.Drive", QDBusConnection::systemBus());

    if ((numberRE.indexIn(object_path.path()) >= 0 && !object_path.path().startsWith("/org/freedesktop/UDisks2/block_devices/mmcblk")) ||
        mmcRE.indexIn(object_path.path()) >= 0) {
        return QDBusObjectPath();
    }

    if (!driveId.path().isEmpty() && driveId.path() != "/") {
        bool portable = driveInterface.property("Removable").toBool();
        bool optical = driveInterface.property("Optical").toBool();
        bool containsMedia = driveInterface.property("MediaAvailable").toBool();
        QString connectionBus = driveInterface.property("ConnectionBus").toString().toLower();
        bool isValid = containsMedia && !optical && (portable || connectionBus == "usb");

        QString vendor = driveInterface.property("Vendor").toString();
        QString model = driveInterface.property("Model").toString();
        uint64_t size = driveInterface.property("Size").toULongLong();
        bool isoLayout = interfaces_and_properties["org.freedesktop.UDisks2.Block"]["IdType"].toString() == "iso9660";

        QString name;
        if (vendor.isEmpty()) {
            if (model.isEmpty())
                name = interfaces_and_properties["org.freedesktop.UDisks2.Block"]["Device"].toByteArray();
            else
                name = model;
        } else {
            if (model.isEmpty()) {
                name = vendor;
            } else {
                name = QString("%1 %2").arg(vendor).arg(model);
            }
        }

        qDebug() << this->metaObject()->className() << "New drive" << driveId.path() << "-" << name << "(" << size << "bytes;" << (isValid ? "removable;" : "nonremovable;") << connectionBus << ")";

        if (isValid) {
            if (m_drives.contains(object_path)) {
                m_drives[object_path]->updateDrive(name, size, isoLayout);
                return object_path;
            }
            LinuxDrive *d = new LinuxDrive(this, object_path.path(), name, size, isoLayout);
            m_drives[object_path] = d;
            emit DriveProvider::driveConnected(d);

            return object_path;
        }
    }
    return QDBusObjectPath();
}

void LinuxDriveProvider::init(QDBusPendingCallWatcher *watcher) {
    qDebug() << this->metaObject()->className() << "Got a reply to GetManagedObjects, parsing";

    QDBusPendingReply<DBusIntrospection> reply = *watcher;
    //QSet<QDBusObjectPath> oldPaths = m_drives.keys().toSet();
    QSet<QDBusObjectPath> oldPaths = m_drives.keys().isEmpty() ?
                QSet<QDBusObjectPath>() :
                QSet<QDBusObjectPath>(m_drives.keys().begin(), m_drives.keys().end());
    QSet<QDBusObjectPath> newPaths;

    if (reply.isError()) {
        qDebug() << "Could not read drives from UDisks:" << reply.error().name() << reply.error().message();
        emit backendBroken(tr("UDisks2 seems to be unavailable or unaccessible on your system."));
        return;
    }

    DBusIntrospection introspection = reply.argumentAt<0>();
    for (const QDBusObjectPath &i : introspection.keys()) {
        if (!i.path().startsWith("/org/freedesktop/UDisks2/block_devices")) {
            continue;
        }

        QDBusObjectPath path = handleObject(i, introspection[i]);
        if (!path.path().isEmpty()) {
            newPaths.insert(path);
        }
    }

    for (const QDBusObjectPath &i : oldPaths - newPaths) {
        emit driveRemoved(m_drives[i]);
        m_drives[i]->deleteLater();
        m_drives.remove(i);
    }

    m_initialized = true;
    emit initializedChanged();
}

void LinuxDriveProvider::onInterfacesAdded(const QDBusObjectPath &object_path, const InterfacesAndProperties &interfaces_and_properties) {
    QRegExp numberRE("[0-9]$");

    if (interfaces_and_properties.keys().contains("org.freedesktop.UDisks2.Block")) {
        if (!m_drives.contains(object_path)) {
            handleObject(object_path, interfaces_and_properties);
        }
    }
}

void LinuxDriveProvider::onInterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces) {
    if (interfaces.contains("org.freedesktop.UDisks2.Block")) {
        if (m_drives.contains(object_path)) {
            qDebug() << this->metaObject()->className() << "Drive at" << object_path.path() << "removed";
            emit driveRemoved(m_drives[object_path]);
            m_drives[object_path]->deleteLater();
            m_drives.remove(object_path);
        }
    }
}

void LinuxDriveProvider::onPropertiesChanged(const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties) {
    Q_UNUSED(interface_name)
    const QSet<QString> watchedProperties = {"MediaAvailable", "Size"};

    // not ideal but it works alright without a huge lot of code
    QSet<QString> cProp = changed_properties.keys().isEmpty() ?
                QSet<QString>() :
                QSet<QString>(changed_properties.keys().begin(), changed_properties.keys().end());
    QSet<QString> iProp = invalidated_properties.isEmpty() ?
                QSet<QString>() :
                QSet<QString>(invalidated_properties.begin(), invalidated_properties.end());
    if (!cProp.intersect(watchedProperties).isEmpty() ||
        !iProp.intersect(watchedProperties).isEmpty()) {
        QDBusPendingCall pcall = m_objManager->asyncCall("GetManagedObjects");
        QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(pcall, this);

        connect(w, &QDBusPendingCallWatcher::finished, this, &LinuxDriveProvider::init);
    }
}

LinuxDrive::LinuxDrive(LinuxDriveProvider *parent, const QString &device, const QString &name, const uint64_t size, const bool isoLayout)
: Drive(parent, name, size, isoLayout) {
    m_device = device;
    m_process = nullptr;
}

LinuxDrive::~LinuxDrive() {
    if (m_variant && m_variant->status() == Variant::WRITING) {
        m_variant->setErrorString(tr("The drive was removed while it was written to."));
        m_variant->setStatus(Variant::WRITING_FAILED);
    }
}

bool LinuxDrive::write(Variant *variant) {
    qDebug() << this->metaObject()->className() << "Will now write" << variant->fileName() << "to" << this->m_device;

    if (!Drive::write(variant)) {
        return false;
    }

    if (!m_process) {
        m_process = new QProcess(this);
    }

    const QString helperPath = getHelperPath();
    if (!helperPath.isEmpty()) {
        m_process->setProgram(helperPath);
    } else {
        variant->setErrorString(tr("Could not find the helper binary. Check your installation."));
        variant->setStatus(Variant::WRITING_FAILED);
        return false;
    }

    QStringList args;
    args << "write";
    args << variant->filePath();
    args << m_device;
    args << variant->md5sum();

    qDebug() << this->metaObject()->className() << "Helper command will be" << args;
    m_process->setArguments(args);

    connect(m_process, &QProcess::readyRead, this, &LinuxDrive::onReadyRead);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
#if QT_VERSION >= 0x050600
    // TODO check if this is actually necessary - it should work just fine even without it
    connect(m_process, &QProcess::errorOccurred, this, &LinuxDrive::onErrorOccurred);
#endif

    m_process->start(QIODevice::ReadOnly);

    return true;
}

void LinuxDrive::cancel() {
    Drive::cancel();
    static bool beingCancelled = false;
    if (m_process != nullptr && !beingCancelled) {
        beingCancelled = true;
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
        beingCancelled = false;
    }
}

void LinuxDrive::restore() {
    qDebug() << this->metaObject()->className() << "Will now restore" << this->m_device;

    if (!m_process) {
        m_process = new QProcess(this);
    }

    m_restoreStatus = RESTORING;
    emit restoreStatusChanged();

    const QString helperPath = getHelperPath();
    if (!helperPath.isEmpty()) {
        m_process->setProgram(helperPath);
    } else {
        qDebug() << "Couldn't find the helper binary.";
        setRestoreStatus(RESTORE_ERROR);
        return;
    }

    QStringList args;
    args << "restore";
    args << m_device;
    qDebug() << this->metaObject()->className() << "Helper command will be" << args;
    m_process->setArguments(args);

    connect(m_process, &QProcess::readyRead, this, &LinuxDrive::onReadyRead);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onRestoreFinished(int, QProcess::ExitStatus)));

    m_process->start(QIODevice::ReadOnly);
}

void LinuxDrive::onReadyRead() {
    if (!m_process) {
        return;
    }

    m_progress->setCurrent(NAN);

    if (m_variant->status() != Variant::WRITE_VERIFYING && m_variant->status() != Variant::WRITING) {
        m_variant->setStatus(Variant::WRITING);
    }

    while (m_process->bytesAvailable() > 0) {
        QString line = m_process->readLine().trimmed();
        qDebug() << "helper:" << line;
        if (line == "WRITE") {
            // Set progress bar max value at start of writing
            const QFile file(m_variant->filePath());
            m_progress->setMax(file.size());

            m_progress->setCurrent(0);

            m_variant->setStatus(Variant::WRITING);
        } else if (line == "CHECK") {
            qDebug() << this->metaObject()->className() << "Helper finished writing, now it will check the written data";
            const QFile file(m_variant->filePath());
            m_progress->setMax(file.size());
            m_progress->setCurrent(0);
            m_variant->setStatus(Variant::WRITE_VERIFYING);
        } else if (line == "DONE") {
            m_variant->setStatus(Variant::WRITING_FINISHED);
            Notifications::notify(tr("Finished!"), tr("Writing %1 was successful").arg(m_variant->fileName()));
        } else {
            bool ok = false;
            qreal val = line.toULongLong(&ok);
            if (ok && val > 0.0) {
                m_progress->setCurrent(val);
            }
        }
    }
}

void LinuxDrive::onFinished(const int exitCode, const QProcess::ExitStatus status) {
    qDebug() << this->metaObject()->className() << "Helper process finished with status" << status;

    if (!m_process) {
        return;
    }

    if (exitCode != 0) {
        QString errorMessage = m_process->readAllStandardError();
        qDebug() << "Writing failed:" << errorMessage;
        Notifications::notify(tr("Error"), tr("Writing %1 failed").arg(m_variant->fileName()));

        m_variant->setErrorString(errorMessage);

        if (m_variant->status() == Variant::WRITE_VERIFYING) {
            m_variant->setStatus(Variant::WRITE_VERIFYING_FAILED);
        } else {
            m_variant->setStatus(Variant::WRITING_FAILED);
        }
    } else {
        Notifications::notify(tr("Finished!"), tr("Writing %1 was successful").arg(m_variant->fileName()));
        m_variant->setStatus(Variant::WRITING_FINISHED);
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
        m_variant = nullptr;
    }
}

void LinuxDrive::onRestoreFinished(const int exitCode, const QProcess::ExitStatus status) {
    qDebug() << this->metaObject()->className() << "Helper process finished with status" << status;

    if (exitCode != 0) {
        if (m_process) {
            qDebug() << "Drive restoration failed:" << m_process->readAllStandardError();
        } else {
            qDebug() << "Drive restoration failed";
        }
        m_restoreStatus = RESTORE_ERROR;
    } else {
        m_restoreStatus = RESTORED;
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    emit restoreStatusChanged();
}

void LinuxDrive::onErrorOccurred(const QProcess::ProcessError e) {
    Q_UNUSED(e);
    if (!m_process) {
        return;
    }

    QString errorMessage = m_process->errorString();
    qDebug() << "Restoring failed:" << errorMessage;
    m_variant->setErrorString(errorMessage);
    m_process->deleteLater();
    m_process = nullptr;
    m_variant->setStatus(Variant::WRITING_FAILED);
    m_variant = nullptr;
}

QString LinuxDrive::devicePath() const {
    QString deviceName = m_device.mid(m_device.lastIndexOf("/"));
    return "/dev" + deviceName;
}
