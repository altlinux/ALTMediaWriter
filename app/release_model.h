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

#ifndef RELEASE_MODEL_H
#define RELEASE_MODEL_H

/*
 * ReleaseModel stores releases and ReleaseFilterModel filters them.
 * While the filter is on the "front page", a small number of releases
 * is shown without filtering. Outside the front page all releases are
 * shown and filtering is enabled. Releases can be filtered by name
 * and/or architecture. Filtering by architecture shows those releases
 * that contain a variant with matching architecture.
 */

#include "architecture.h"

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

class Release;

class ReleaseModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    Release *get(const int index) const;
    QHash<int, QByteArray> roleNames() const override;
};

class ReleaseFilterModel final : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(bool frontPage READ getFrontPage NOTIFY frontPageChanged)

public:
    ReleaseFilterModel(ReleaseModel *model_arg, QObject *parent);

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void invalidateCustom();

    bool getFrontPage() const;
    Q_INVOKABLE void leaveFrontPage();
    Q_INVOKABLE void setFilterArch(const int index);
    Q_INVOKABLE void setFilterText(const QString &text);

signals:
    void frontPageChanged();

private:
    ReleaseModel *model;
    bool frontPage;
    QString filterText;
    Architecture filterArch;
};

#endif // RELEASE_MODEL_H
