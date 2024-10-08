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

#ifndef WINDRIVEMANAGER_H
#define WINDRIVEMANAGER_H

#include "drivemanager.h"

#include <QProcess>

class WinDriveProvider;
class WinDrive;
class Variant;

class WinDriveProvider : public DriveProvider {
    Q_OBJECT
public:
    WinDriveProvider(DriveManager *parent);

public slots:
    void checkDrives();

private:
    QSet<int> findPhysicalDrive(const char driveLetter);
    bool describeDrive(const int nDriveNumber, const bool hasLetter, const bool verbose);

    QMap<int, WinDrive *> m_drives;
};

class WinDrive : public Drive {
    Q_OBJECT
public:
    WinDrive(WinDriveProvider *parent, const QString &name, const uint64_t size, const bool containsLive, const int device, const QString &serialNumber);
    ~WinDrive();

    Q_INVOKABLE virtual bool write(Variant *variant) override;
    Q_INVOKABLE virtual void cancel() override;
    Q_INVOKABLE virtual void restore() override;

    QString serialNumber() const;

    bool operator==(const WinDrive &other) const;

private slots:
    void onFinished(const int exitCode, const QProcess::ExitStatus exitStatus);
    void onRestoreFinished(const int exitCode, const QProcess::ExitStatus exitStatus);
    void onReadyRead();

private:
    int m_device;
    QString m_serialNo;
    QProcess *m_child;
};

#endif // WINDRIVEMANAGER_H
