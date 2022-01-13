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

#include "release_model.h"
#include "release.h"
#include "variant.h"

Release *ReleaseModel::get(const int index) const {
    QStandardItem *the_item = item(index);
    
    if (the_item != nullptr) {
        QVariant variant = the_item->data();
        Release *release = variant.value<Release *>();

        return release;
    } else {
        return nullptr;
    }
}

// NOTE: this is a very roundabout way of making the Release
// pointer stored inside the item available in the qml
// delegate as "release". (Qt::UserRole + 1 is the default
// data role)
QHash<int, QByteArray> ReleaseModel::roleNames() const {
    static const QHash<int, QByteArray> names = {
        {Qt::UserRole + 1, "release"}
    };

    return names;
}

ReleaseFilterModel::ReleaseFilterModel(ReleaseModel *model_arg, QObject *parent)
: QSortFilterProxyModel(parent)
{
    model = model_arg;
    frontPage = true;
    filterArch = Architecture_ALL;

    setSourceModel(model_arg);
}

bool ReleaseFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const {

    Release *release = model->get(source_row);
    if (release == nullptr) {
        return false;
    }
    
    if (frontPage) {
        // Don't filter when on front page, just show 3 releases
        const bool on_front_page = (source_row < 3);

        return on_front_page;
    } else if (release->isCustom()) {
        // Always show local release
        return true;
    } else {
        const bool releaseMatchesName = release->displayName().contains(filterText, Qt::CaseInsensitive);

        // Exit early if don't match name to skip checking
        // for arch because that takes a long time
        // TODO: cache that somehow?
        if (!releaseMatchesName) {
            return false;
        }

        // Otherwise filter by arch
        const bool releaseHasVariantWithArch = [this, release]() {
            // If filtering for all, accept all architectures
            if (filterArch == Architecture_ALL) {
                return true;
            }

            for (const Variant *variant : release->variantList()) {
                if (variant->arch() == filterArch) {
                    return true;
                }
            }
            return false;
        }();

        if (!releaseHasVariantWithArch) {
            return false;
        }

        return true;
    }
}

bool ReleaseFilterModel::getFrontPage() const {
    return frontPage;
}

void ReleaseFilterModel::leaveFrontPage() {
    frontPage = false;

    invalidateFilter();

    emit frontPageChanged();
}

void ReleaseFilterModel::setFilterText(const QString &text) {
    filterText = text;

    invalidateFilter();
}

void ReleaseFilterModel::setFilterArch(const int index) {
    filterArch = (Architecture) index;
    invalidateFilter();
}

void ReleaseFilterModel::invalidateCustom() {
    // NOTE: need this because public invalidate() doesn't work completely for some reason
    invalidateFilter();
}
