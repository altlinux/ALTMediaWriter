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

#ifndef RELEASEMANAGER_H
#define RELEASEMANAGER_H

/*
 * ReleaseManager - a singleton that provides access to releases in
 * the qml portion of the app.
 */

#include <QObject>

class Release;
class ReleaseModel;
class ReleaseFilterModel;

class ReleaseManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool downloadingMetadata READ downloadingMetadata NOTIFY downloadingMetadataChanged)

    Q_PROPERTY(Release *selected READ selected NOTIFY selectedChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedChanged)

    Q_PROPERTY(QStringList architectures READ architectures CONSTANT)
    Q_PROPERTY(QStringList fileTypeFilters READ fileTypeFilters CONSTANT)

    Q_PROPERTY(ReleaseFilterModel *filter READ getFilterModel CONSTANT)

public:
    explicit ReleaseManager(QObject *parent = 0);

    bool downloadingMetadata() const;

    QStringList architectures() const;
    QStringList fileTypeFilters() const;

    Release *selected() const;
    int selectedIndex() const;
    void setSelectedIndex(const int index);

    ReleaseFilterModel *getFilterModel() const;

signals:
    void downloadingMetadataChanged();
    void selectedChanged();

private:
    ReleaseModel *sourceModel;
    ReleaseFilterModel *filterModel;
    int m_selectedIndex;
    bool m_downloadingMetadata;

    void loadVariants(const QString &variantsFile);
    void setDownloadingMetadata(const bool value);
    void downloadMetadata();
    void loadReleases(const QList<QString> &sectionsFiles);
    void addReleaseToModel(const int index, Release *release);
};

#endif // RELEASEMANAGER_H
