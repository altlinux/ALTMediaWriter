/*
 * ALT Media Writer
 * Copyright (C) 2022 Jan Grulich <jgrulich@redhat.com>
 * Copyright (C) 2011-2022 Pete Batard <pete@akeo.ie>
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

#include "windrivemanager.h"
#include "notifications.h"
#include "progress.h"
#include "variant.h"

#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QVector>

#include <windows.h>
#define INITGUID
#include <guiddef.h>

#include <cmath>
#include <cstring>

DEFINE_GUID(PARTITION_MICROSOFT_DATA, 0xEBD0A0A2, 0xB9E5, 0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7);

static QString getPhysicalName(const int driveNumber) {
    return QString("\\\\.\\PhysicalDrive%0").arg(driveNumber);
}

static HANDLE getPhysicalHandle(const int driveNumber) {
    const QString physicalPath = getPhysicalName(driveNumber);
    return CreateFileA(physicalPath.toStdString().c_str(), GENERIC_READ, FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

WinDriveProvider::WinDriveProvider(DriveManager *parent)
: DriveProvider(parent) {
    qDebug() << this->metaObject()->className() << "construction";
    QTimer::singleShot(0, this, &WinDriveProvider::checkDrives);
}

void WinDriveProvider::checkDrives() {
    static bool firstRun = true;
    if (firstRun) {
        qDebug() << this->metaObject()->className() << "Looking for the drives for the first time";
    }

    for (int i = 0; i < 64; i++) {
        bool present = describeDrive(i, firstRun);
        if (!present && m_drives.contains(i)) {
            emit driveRemoved(m_drives[i]);
            m_drives[i]->deleteLater();
            m_drives.remove(i);
        }
    }

    if (firstRun) {
        qDebug() << this->metaObject()->className() << "Finished looking for the drives for the first time";
    }
    firstRun = false;
    QTimer::singleShot(2500, this, &WinDriveProvider::checkDrives);
}

bool WinDriveProvider::isMountable(const int driveNumber) {
    qDebug() << this->metaObject()->className() << "Checking whether" << getPhysicalName(driveNumber) << "is mountable";

    HANDLE physicalHandle = getPhysicalHandle(driveNumber);
    if (physicalHandle == INVALID_HANDLE_VALUE) {
        qDebug() << this->metaObject()->className() << "Could not get physical handle for drive" << getPhysicalName(driveNumber);
        return false;
    }

    DWORD size = 0;
    BYTE geometry[256];
    bool result = DeviceIoControl(physicalHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL, 0, geometry, sizeof(geometry), &size, NULL);
    if (!result || size == 0) {
        qDebug() << this->metaObject()->className() << "Could not get geometry for drive" << getPhysicalName(driveNumber);
        CloseHandle(physicalHandle);
        return false;
    }

    BYTE layout[4096] = {0};
    result = DeviceIoControl(physicalHandle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
        NULL, 0, layout, sizeof(layout), &size, NULL);
    if (!result || size == 0) {
        qDebug() << this->metaObject()->className() << "Could not get layout for drive" << getPhysicalName(driveNumber);
        CloseHandle(physicalHandle);
        return false;
    }

    PDRIVE_LAYOUT_INFORMATION_EX driveLayout = reinterpret_cast<PDRIVE_LAYOUT_INFORMATION_EX>(layout);

    switch (driveLayout->PartitionStyle) {
        case PARTITION_STYLE_MBR: {
            qDebug() << this->metaObject()->className() << "MBR partition style";
            const QVector<BYTE> mountablePartitionTypes = {0x01, 0x04, 0x06, 0x07, 0x0b, 0x0c, 0x0e};
            for (DWORD i = 0; i < driveLayout->PartitionCount; ++i) {
                const BYTE partitionType = driveLayout->PartitionEntry[i].Mbr.PartitionType;
                if (partitionType == PARTITION_ENTRY_UNUSED) {
                    continue;
                }

                qDebug() << this->metaObject()->className() << "Partition type:" << partitionType;
                if (!mountablePartitionTypes.contains(partitionType)) {
                    CloseHandle(physicalHandle);
                    qDebug() << this->metaObject()->className() << getPhysicalName(driveNumber) << "is not mountable";
                    return false;
                }
            }
            break;
        }
        case PARTITION_STYLE_GPT:
            qDebug() << this->metaObject()->className() << "GPT partition style";
            for (DWORD i = 0; i < driveLayout->PartitionCount; ++i) {
                if (std::memcmp(&driveLayout->PartitionEntry[i].Gpt.PartitionType,
                        &PARTITION_MICROSOFT_DATA, sizeof(GUID)) != 0) {
                    CloseHandle(physicalHandle);
                    qDebug() << this->metaObject()->className() << getPhysicalName(driveNumber) << "is not mountable";
                    return false;
                }
            }
            break;
        default:
            qDebug() << this->metaObject()->className() << "Partition type: RAW";
            break;
    }

    qDebug() << this->metaObject()->className() << getPhysicalName(driveNumber) << "is mountable";
    CloseHandle(physicalHandle);
    return true;
}

bool WinDriveProvider::describeDrive(const int nDriveNumber, const bool verbose) {
    BOOL removable;
    QString productVendor;
    QString productId;
    QString serialNumber;
    uint64_t deviceBytes;
    STORAGE_BUS_TYPE storageBus;

    BOOL bResult = FALSE; // results flag
    //DWORD dwRet = NO_ERROR;

    // Format physical drive path (may be '\\.\PhysicalDrive0', '\\.\PhysicalDrive1' and so on).
    QString strDrivePath = getPhysicalName(nDriveNumber);

    // Get a handle to physical drive
    HANDLE hDevice = ::CreateFile(strDrivePath.toStdWString().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        return false; //::GetLastError();
    }

    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "is present";
    }

    // Set the input data structure
    STORAGE_PROPERTY_QUERY storagePropertyQuery;
    ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
    storagePropertyQuery.PropertyId = StorageDeviceProperty;
    storagePropertyQuery.QueryType = PropertyStandardQuery;

    // Get the necessary output buffer size
    STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader;
    ZeroMemory(&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER));
    DWORD dwBytesReturned = 0;
    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "IOCTL_STORAGE_QUERY_PROPERTY";
    }
    if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
            &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
            &storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
            &dwBytesReturned, NULL)) {
        //dwRet = ::GetLastError();
        ::CloseHandle(hDevice);
        return false; // dwRet;
    }

    // Alloc the output buffer
    const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
    BYTE *pOutBuffer = new BYTE[dwOutBufferSize];
    ZeroMemory(pOutBuffer, dwOutBufferSize);

    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "IOCTL_STORAGE_QUERY_PROPERTY with a bigger buffer";
    }
    // Get the storage device descriptor
    if (!(bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
              &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
              pOutBuffer, dwOutBufferSize,
              &dwBytesReturned, NULL))) {
        //dwRet = ::GetLastError();
        delete[] pOutBuffer;
        ::CloseHandle(hDevice);
        return false; // dwRet;
    }

    // Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
    // followed by additional info like vendor ID, product ID, serial number, and so on.
    STORAGE_DEVICE_DESCRIPTOR *pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR *) pOutBuffer;
    removable = pDeviceDescriptor->RemovableMedia;
    if (pDeviceDescriptor->ProductIdOffset != 0) {
        productId = QString((char *) pOutBuffer + pDeviceDescriptor->ProductIdOffset).trimmed();
    }
    if (pDeviceDescriptor->VendorIdOffset != 0) {
        productVendor = QString((char *) pOutBuffer + pDeviceDescriptor->VendorIdOffset).trimmed();
    }
    if (pDeviceDescriptor->SerialNumberOffset != 0) {
        serialNumber = QString((char *) pOutBuffer + pDeviceDescriptor->SerialNumberOffset).trimmed();
    }
    storageBus = pDeviceDescriptor->BusType;

    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "detected:" << productVendor << productId << (removable ? ", removable" : ", nonremovable") << (storageBus == BusTypeUsb ? "USB" : "notUSB");
    }

    if (!removable && storageBus != BusTypeUsb) {
        return false;
    }

    DISK_GEOMETRY pdg;
    DWORD junk = 0; // discard results

    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "IOCTL_DISK_GET_DRIVE_GEOMETRY";
    }

    bResult = DeviceIoControl(hDevice, // device to be queried
        IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
        NULL, 0, // no input buffer
        &pdg, sizeof(pdg), // output buffer
        &junk, // # bytes returned
        (LPOVERLAPPED) NULL); // synchronous I/O

    if (!bResult || pdg.MediaType == Unknown) {
        return false;
    }

    deviceBytes = pdg.Cylinders.QuadPart * pdg.TracksPerCylinder * pdg.SectorsPerTrack * pdg.BytesPerSector;

    // Do cleanup and return
    if (verbose) {
        qDebug() << this->metaObject()->className() << strDrivePath << "cleanup, adding to the list";
    }
    delete[] pOutBuffer;
    ::CloseHandle(hDevice);

    WinDrive *currentDrive = new WinDrive(this, productVendor + " " + productId, deviceBytes, !isMountable(nDriveNumber), nDriveNumber, serialNumber);
    if (m_drives.contains(nDriveNumber) && *m_drives[nDriveNumber] == *currentDrive) {
        currentDrive->deleteLater();
        return true;
    }

    if (m_drives.contains(nDriveNumber)) {
        emit driveRemoved(m_drives[nDriveNumber]);
        m_drives[nDriveNumber]->deleteLater();
    }

    m_drives[nDriveNumber] = currentDrive;
    emit driveConnected(currentDrive);

    return true;
}

WinDrive::WinDrive(WinDriveProvider *parent, const QString &name, const uint64_t size, const bool containsLive, const int device, const QString &serialNumber)
: Drive(parent, name, size, containsLive) {
    m_device = device;
    m_serialNo = serialNumber;
    m_child = nullptr;
}

WinDrive::~WinDrive() {
    if (m_child) {
        m_child->kill();
    }
}

bool WinDrive::write(Variant *variant) {
    qDebug() << this->metaObject()->className() << "Preparing to write" << variant->fileName() << "to drive" << m_device;
    if (!Drive::write(variant)) {
        return false;
    }

    if (m_child) {
        // TODO some handling of an already present process
        m_child->deleteLater();
    }
    m_child = new QProcess(this);
    connect(m_child, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &WinDrive::onFinished);
    connect(m_child, &QProcess::readyRead, this, &WinDrive::onReadyRead);

    const QString helperPath = getHelperPath();
    if (!helperPath.isEmpty()) {
        m_child->setProgram(helperPath);
    } else {
        variant->setErrorString(tr("Could not find the helper binary. Check your installation."));
        return false;
    }

    QStringList args;
    args << "write";
    args << variant->filePath();
    args << QString("%1").arg(m_device);
    args << variant->md5sum();
    m_child->setArguments(args);

    qDebug() << this->metaObject()->className() << "Starting" << m_child->program() << args;
    m_child->start();
    return true;
}

void WinDrive::cancel() {
    Drive::cancel();
    if (m_child) {
        m_child->kill();
        m_child->deleteLater();
        m_child = nullptr;
    }
}

void WinDrive::restore() {
    qDebug() << this->metaObject()->className() << "Preparing to restore disk" << m_device;
    if (m_child) {
        m_child->deleteLater();
    }

    m_child = new QProcess(this);

    m_restoreStatus = RESTORING;
    emit restoreStatusChanged();

    const QString helperPath = getHelperPath();
    if (!helperPath.isEmpty()) {
        m_child->setProgram(helperPath);
    } else {
        m_restoreStatus = RESTORE_ERROR;
        return;
    }

    QStringList args;
    args << "restore";
    args << QString("%1").arg(m_device);
    m_child->setArguments(args);

    //connect(m_process, &QProcess::readyRead, this, &LinuxDrive::onReadyRead);
    connect(m_child, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onRestoreFinished(int, QProcess::ExitStatus)));

    qDebug() << this->metaObject()->className() << "Starting" << m_child->program() << args;

    m_child->start(QIODevice::ReadOnly);
}

QString WinDrive::serialNumber() const {
    return m_serialNo;
}

bool WinDrive::operator==(const WinDrive &other) const {
    return (other.serialNumber() == serialNumber()) && Drive::operator==(other);
}

void WinDrive::onFinished(const int exitCode, const QProcess::ExitStatus exitStatus) {
    if (!m_child) {
        return;
    }

    qDebug() << "Child finished" << exitCode << exitStatus;
    qDebug() << m_child->errorString();

    if (exitCode == 0) {
        m_variant->setStatus(Variant::WRITING_FINISHED);
        Notifications::notify(tr("Finished!"), tr("Writing %1 was successful").arg(m_variant->fileName()));
    } else {
        m_variant->setErrorString(QString::fromLocal8Bit(m_child->readAllStandardError()).trimmed());

        if (m_variant->status() == Variant::WRITE_VERIFYING) {
            m_variant->setStatus(Variant::WRITE_VERIFYING_FAILED);
        } else {
            m_variant->setStatus(Variant::WRITING_FAILED);
        }
    }

    m_child->deleteLater();
    m_child = nullptr;
}

void WinDrive::onRestoreFinished(const int exitCode, const QProcess::ExitStatus exitStatus) {
    if (!m_child) {
        return;
    }

    qDebug() << "Process finished" << exitCode << exitStatus;
    qDebug() << m_child->readAllStandardError();

    if (exitCode == 0) {
        m_restoreStatus = RESTORED;
    } else {
        m_restoreStatus = RESTORE_ERROR;
    }
    emit restoreStatusChanged();

    m_child->deleteLater();
    m_child = nullptr;
}

void WinDrive::onReadyRead() {
    if (!m_child) {
        return;
    }

    m_progress->setCurrent(NAN);

    if (m_variant->status() != Variant::WRITE_VERIFYING && m_variant->status() != Variant::WRITING) {
        m_variant->setStatus(Variant::WRITING);
    }

    while (m_child->bytesAvailable() > 0) {
        QString line = m_child->readLine().trimmed();
        if (line == "WRITE") {
            m_progress->setCurrent(0);

            // Set progress bar max value at start of writing
            const QFile file(m_variant->filePath());
            m_progress->setMax(file.size());
        } else if (line == "DONE") {
            m_variant->setStatus(Variant::WRITING_FINISHED);
            Notifications::notify(tr("Finished!"), tr("Writing %1 was successful").arg(m_variant->fileName()));
        } else if (line == "CHECK") {
            qDebug() << this->metaObject()->className() << "Written media check starting";
            const QFile file(m_variant->filePath());
            m_progress->setMax(file.size());
            m_progress->setCurrent(0);
            m_variant->setStatus(Variant::WRITE_VERIFYING);
        } else {
            bool ok;
            qreal bytes = line.toLongLong(&ok);
            if (ok) {
                if (bytes < 0) {
                    m_progress->setCurrent(NAN);
                } else {
                    m_progress->setCurrent(bytes);
                }
            }
        }
    }
}
