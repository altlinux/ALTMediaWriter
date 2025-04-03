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

#ifndef RELEASE_H
#define RELEASE_H

/**
 * @brief The Release class
 *
 * Represents a group of images, for example "Workstation", "Server"
 * or "Education". A release is further divided into Variants.
 *
 * @property name the codename of the release, as stored in metadata
 * @property displayName the translated name for display
 * @property summary the summary describing the release, displayed on
 *     the main screen
 * @property description the extensive description of the release -
 *     displayed on the detail screen
 * @property isCustom true if this is the custom release
 * @property icon path of the icon of this release
 * @property screenshots a list of paths to screenshots (typically
 *     HTTP URLs)
 * @property variants a list of available variants for this release
 * @property variant currently selected variant
 * @property variantIndex the index of the currently selected variant
 */

#include "architecture.h"
#include "platform.h"
#include "file_type.h"

#include <QQmlListProperty>
#include <QUrl>
#include <map>

struct str_num_compare {
    bool operator() (const QString& lhs, const QString& rhs) const {
        return lhs.mid(1).toInt() > rhs.mid(1).toInt();
    }
};

class Variant;

class Release : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString summary READ summary CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)

    Q_PROPERTY(bool isCustom READ isCustom CONSTANT)

    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QStringList screenshots READ screenshots CONSTANT)

    Q_PROPERTY(QQmlListProperty<Variant> variants READ variants NOTIFY variantsChanged)
    Q_PROPERTY(Variant *variant READ selectedVariant NOTIFY selectedVariantChanged)
    Q_PROPERTY(int variantIndex READ selectedVariantIndex WRITE setSelectedVariantIndex NOTIFY selectedVariantChanged)

    Q_PROPERTY(QString selectedPlatform READ selectedPlatform WRITE setSelectedPlatform NOTIFY selectedPlatformChanged)
    Q_PROPERTY(int countAddIndex READ countAddIndex CONSTANT)

    Q_PROPERTY(QStringList platformsList READ platformsList CONSTANT)
    Q_PROPERTY(QVariantList filteredVariantsPlatform READ filteredVariantsPlatform NOTIFY selectedPlatformChanged)
public:
    Release(const QString &name, const QString &displayName, const QString &summary, const QString &description, const QString &icon, const QStringList &screenshots, QObject *parent);

    static Release *custom(QObject *parent);

    void addVariant(Variant *variant);

    Q_INVOKABLE void setLocalFile(const QUrl &fileUrl);

    QString name() const;
    QString displayName() const;
    QString summary() const;
    QString description() const;
    bool isCustom() const;
    QString icon() const;
    QStringList screenshots() const;

    QQmlListProperty<Variant> variants();
    QList<Variant *> variantList() const;
    Variant *selectedVariant() const;
    int selectedVariantIndex() const;
    void setSelectedVariantIndex(const int index);

    QStringList platformsList() const;
    QVariantList filteredVariantsPlatform () const;
    int countAddIndex() const;

    Platform selectedPlatform() const;
    void setSelectedPlatform (const QString &platform_name);

signals:
    void variantsChanged();
    void selectedVariantChanged();
    void selectedPlatformChanged();

private:
    QString m_name;
    QString m_displayName;
    QString m_summary;
    QString m_description;
    QString m_icon;
    QStringList m_screenshots;
    QList<Variant *> m_variants;
    Platform m_selectedPlatform;
    int m_selectedVariant;
    bool m_isCustom;

    static bool customSort (const QString &a, const QString &b);
    std::map<QString, int, str_num_compare> platform_members_count;
};

#endif // RELEASE_H
