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

#include "releasemanager.h"
#include "release_model.h"
#include "release.h"
#include "file_type.h"
#include "architecture.h"
#include "variant.h"
#include "network.h"

#include <yaml-cpp/yaml.h>

#include <QtQml>
#include <QApplication>
#include <QAbstractEventDispatcher>

QList<QString> load_list_from_file(const QString &filepath);
QList<QString> get_sections_urls();
QList<QString> get_images_urls();
QString yml_get(const YAML::Node &node, const QString &key);

ReleaseManager::ReleaseManager(QObject *parent)
: QObject(parent)
, m_downloadingMetadata(true)
{
    qDebug() << this->metaObject()->className() << "construction";

    sourceModel = new ReleaseModel(this);
    filterModel = new ReleaseFilterModel(sourceModel, this);

    // Add custom release to first position
    Release *customRelease = Release::custom(this);
    addReleaseToModel(0, customRelease);
    setSelectedIndex(0);

    QTimer::singleShot(0, this, &ReleaseManager::downloadMetadata);
}

void ReleaseManager::downloadMetadata() {
    qDebug() << "Downloading metadata";
    
    setDownloadingMetadata(true);

    const QList<QString> section_urls = get_sections_urls();
    qDebug() << "section_urls = " << section_urls;

    const QList<QString> image_urls = get_images_urls();
    qDebug() << "image_urls = " << image_urls;

    // Create requests to download all release files and
    // collect the replies
    const QHash<QString, QNetworkReply *> replies =
    [section_urls, image_urls]() {
        QHash<QString, QNetworkReply *> out;
        const QList<QString> all_urls = section_urls + image_urls;

        for (const QString &url : all_urls) {
            QNetworkReply *reply = makeNetworkRequest(url, 5000);

            out[url] = reply;
        }

        return out;
    }();

    // This will run when all the replies are finished (or
    // technically, the last one)
    const auto onReplyFinished =
    [this, replies, section_urls, image_urls]() {
        // Only proceed if this is the last reply
        for (const QNetworkReply *reply : replies) {
            if (!reply->isFinished()) {
                return;
            }
        }

        // Check that all replies suceeded
        // If not, retry
        for (const QNetworkReply *reply : replies.values()) {
            // NOTE: ignore ContentNotFoundError since it can happen if one of the files was moved or renamed
            const QNetworkReply::NetworkError error = reply->error();
            const bool download_failed = (error != QNetworkReply::NoError && error != QNetworkReply::ContentNotFoundError);

            if (download_failed) {
                qDebug() << "Failed to download metadata:" << reply->errorString() << reply->error() << "Retrying in 10 seconds.";
                QTimer::singleShot(10000, this, &ReleaseManager::downloadMetadata);

                return;
            }
        }

        qDebug() << "Downloaded metadata, loading it";

        // Collect results
        QHash<QString, QString> url_to_file;

        for (const QString &url : replies.keys()) {
            QNetworkReply *reply = replies[url];

            if (reply->error() == QNetworkReply::NoError) {
                const QByteArray bytes = reply->readAll();
                url_to_file[url] = QString(bytes);
            } else {
                qDebug() << "Failed to download metadata from" << url;
                qDebug() << "Error:" << reply->error();
            }
        }

        const QList<QString> sectionsFiles =
        [section_urls, url_to_file]() {
            QList<QString> out;
            for (const QString &section_url : section_urls) {
                const QString section = url_to_file[section_url];
                out.append(section);
            }
            return out;
        }();

        qDebug() << "Loading releases";

        loadReleases(sectionsFiles);

        const QList<QString> imagesFiles =
        [image_urls, url_to_file]() {
            QList<QString> out;
            for (const QString &image_url : image_urls) {
                const QString image = url_to_file[image_url];
                out.append(image);
            }
            return out;
        }();

        qDebug() << "Loading variants";

        for (const QString &imagesFile : imagesFiles) {
            loadVariants(imagesFile);
        }

        for (QNetworkReply *reply : replies.values()) {
            reply->deleteLater();
        }

        setDownloadingMetadata(false);
    };

    for (QNetworkReply *reply : replies) {
        connect(
            reply, &QNetworkReply::finished,
            onReplyFinished);
    }
}

void ReleaseManager::setDownloadingMetadata(const bool value) {
    m_downloadingMetadata = value;
    emit downloadingMetadataChanged();
}

bool ReleaseManager::downloadingMetadata() const {
    return m_downloadingMetadata;
}

Release *ReleaseManager::selected() const {
    return sourceModel->get(m_selectedIndex);
}

int ReleaseManager::selectedIndex() const {
    return m_selectedIndex;
}

void ReleaseManager::setSelectedIndex(int row_proxy) {
    const QModelIndex index_proxy = filterModel->index(row_proxy, 0);
    const QModelIndex index_source = filterModel->mapToSource(index_proxy);

    const int row_source = index_source.row();

    if (m_selectedIndex != row_source) {
        m_selectedIndex = row_source;
        emit selectedChanged();
    }
}

ReleaseFilterModel *ReleaseManager::getFilterModel() const {
    return filterModel;
}

void ReleaseManager::loadVariants(const QString &variantsFile) {
    YAML::Node variants = YAML::Load(variantsFile.toStdString());

    if (!variants["entries"]) {
        return;
    }

    for (const YAML::Node &variantData : variants["entries"]) {
        const QString url = yml_get(variantData, "link");
        if (url.isEmpty()) {
            qDebug() << "Variant has no url";
            continue;
        }

        const QString releaseName = yml_get(variantData, "solution");
        if (releaseName.isEmpty()) {
            qDebug() << "Variant has no releaseName" << url;
            continue;
        }

        const QString arch_string = yml_get(variantData, "arch");
        const Architecture arch =
        [arch_string, url]() -> Architecture  {
            if (!arch_string.isEmpty()) {
                return architecture_from_string(arch_string);
            } else {
                return architecture_from_filename(url);
            }
        }();
        if (arch == Architecture_UNKNOWN) {
            qDebug() << "Variant has unknown architecture" << arch_string << url;
            continue;
        }

        // NOTE: yml file doesn't define "board" for pc32/pc64, so default to "PC"
        const QString board =
        [variantData]() -> QString {
            const QString out = yml_get(variantData, "board");
            if (!out.isEmpty()) {
                return out;
            } else {
                return "PC";
            }
        }();

        const FileType fileType = file_type_from_filename(url);
        if (fileType == FileType_UNKNOWN) {
            qDebug() << "Variant has unknown file type" << url;
            continue;
        }

        const bool live =
        [variantData]() {
            const QString live_string = yml_get(variantData, "live");
            if (!live_string.isEmpty()) {
                return (live_string == "1");
            } else {
                return false;
            }
        }();

        // qDebug() << QUrl(url).fileName() << releaseName << architecture_name(arch) << board << file_type_name(fileType) << (live ? "LIVE" : "");

        // Find a release that has the same name as this variant
        Release *release =
        [this, releaseName]() -> Release *{
            for (int i = 0; i < sourceModel->rowCount(); i++) {
                Release *release = sourceModel->get(i);

                if (release->name() == releaseName) {
                    return release;
                }
            }
            return nullptr;
        }();

        if (release != nullptr) {
            Variant *variant = new Variant(url, arch, fileType, board, live, this);
            release->addVariant(variant);
        } else {
            qDebug() << "Failed to find a release for this variant!" << url;
        }
    }
}

QStringList ReleaseManager::architectures() const {
    QStringList out;
    for (const Architecture architecture : architecture_all) {
        if (architecture != Architecture_UNKNOWN) {
            const QString name = architecture_name(architecture);
            out.append(name);
        }
    }
    return out;
}

QStringList ReleaseManager::fileTypeFilters() const {
    const QList<FileType> fileTypes = file_type_all;

    QStringList filters;
    for (const FileType &type : fileTypes) {
        const QString extensions =
        [type]() {
            const QStringList strings = file_type_strings(type);
            if (strings.isEmpty()) {
                return QString();
            }

            QString out;
            out += "(";

            for (const QString &e : strings) {
                if (strings.indexOf(e) > 0) {
                    out += " ";
                }

                out += "*." + e;
            }

            out += ")";

            return out;
        }();

        if (extensions.isEmpty()) {
            continue;
        }

        const QString name = file_type_name(type);
        
        const QString filter = name + " " + extensions;
        filters.append(filter);
    }

    filters.append(tr("All files") + " (*)");

    return filters;
}

void ReleaseManager::loadReleases(const QList<QString> &sectionsFiles) {
    for (const QString &sectionFile : sectionsFiles) {
        const YAML::Node section = YAML::Load(sectionFile.toStdString());

        if (!section["members"]) {
            continue;
        }
        
        for (unsigned int i = 0; i < section["members"].size(); i++) {
            const YAML::Node releaseData = section["members"][i];

            const QString name = yml_get(releaseData, "code");
            if (name.isEmpty()) {
                qDebug() << "Release has no name";
                continue;
            }

            const QString language =
            []() {
                if (QLocale().language() == QLocale::Russian) {
                    return "_ru";
                } else {
                    return "_en";
                }
            }();
            
            const QString display_name = yml_get(releaseData, "name" + language);
            if (display_name.isEmpty()) {
                qDebug() << "Release has no display name";
                continue;
            }

            const QString summary = yml_get(releaseData, "descr" + language);
            if (summary.isEmpty()) {
                qDebug() << "Release has no summary";
                continue;
            }

            const QString description = yml_get(releaseData, "descr_full" + language);
            if (description.isEmpty()) {
                qDebug() << "Release has no description";
                continue;
            }

            // NOTE: currently no screenshots
            const QStringList screenshots;
            
            // Check that icon file exists
            const QString icon_name = yml_get(releaseData, "img");
            if (icon_name.isEmpty()) {
                qDebug() << "Release has no icon";
                continue;
            }

            const QString icon_path_test = ":/logo/" + icon_name;
            const QFile icon_file(icon_path_test);
            if (!icon_file.exists()) {
                qDebug() << "Failed to find icon file at " << icon_path_test << " needed for release " << name;
                continue;
            }

            // NOTE: icon_path is consumed by QML, so it needs to begin with "qrc:/" not ":/"
            const QString icon_path = "qrc" + icon_path_test;

            const auto release = new Release(name, display_name, summary, description, icon_path, screenshots, this);

            // Reorder releases because default order in
            // sections files is not good. Try to put
            // workstation first after custom release and
            // server second, so that they are both on the
            // frontpage.
            const int index =
            [this, release]() {
                const QString release_name = release->name();
                const bool is_workstation = (release_name == "alt-workstation");
                const bool is_kworkstation = (release_name == "alt-kworkstation");

                if (is_workstation) {
                    return 1;
                } else if (is_kworkstation) {
                    return 2;
                } else {
                    return sourceModel->rowCount();
                }
            }();
            
            addReleaseToModel(index, release);
        }
    }

    filterModel->invalidateCustom();
}

void ReleaseManager::addReleaseToModel(const int index, Release *release) {
    QVariant variant;
    variant.setValue(release);

    QStandardItem *item = new QStandardItem();
    item->setData(variant);

    sourceModel->insertRow(index, item);
}

QList<QString> load_list_from_file(const QString &filepath) {
    QFile file(filepath);

    const bool open_success = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!open_success) {
        qDebug() << "Failed to open" << filepath;
        return QList<QString>();
    }

    const QString contents = file.readAll();
    QList<QString> list = contents.split("\n");
    list.removeAll(QString());

    return list;
}

QList<QString> get_sections_urls() {
    static QList<QString> sections_urls = load_list_from_file(":/sections_urls.txt");

    return sections_urls;
}

QList<QString> get_images_urls() {
    static QList<QString> images_urls = load_list_from_file(":/images_urls.txt");

    return images_urls;
}

QString yml_get(const YAML::Node &node, const QString &key) {
    const std::string key_std = key.toStdString();
    const YAML::Node value_yml = node[key_std];
    const std::string fallback = std::string();
    const std::string value_std = value_yml.as<std::string>(fallback);
    QString value = QString::fromStdString(value_std);

    // Remove HTML character entities that don't render in Qt
    value.replace("&colon;", ":");
    value.replace("&nbsp;", " ");
    // Remove newlines because text will have wordwrap
    value.replace("\n", " ");

    return value;
}
