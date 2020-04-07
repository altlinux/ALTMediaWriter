/*
 * Fedora Media Writer
 * Copyright (C) 2016 Martin Bříza <mbriza@redhat.com>
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

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QQmlListProperty>

#include <QDateTime>

#include "downloadmanager.h"

class ReleaseManager;
class ReleaseListModel;
class Release;
class ReleaseVersion;
class ReleaseVariant;
class ReleaseArchitecture;
class ReleaseImageType;
class ReleaseBoard;

/*
 * Architecture - singleton (x86, x86_64, etc)
 *
 * Release -> Version -> Variant
 *
 * Server  -> 24      -> Full    -> x86_64
 *                               -> i686
 *                    -> Netinst -> x86_64
 *                               -> i686
 *         -> 23      -> Full    -> x86_64
 *                               -> i686
 *                    -> Netinst -> x86_64
 *                               -> i686
 *
 * Variant can be downloaded.
 * Variant can be written to a drive - that's handled by the target drive object itself.
 *
 * There should be no platform-dependent code in this file nor in potential child classes.
 */


/**
 * @brief The ReleaseManager class
 *
 * The main entry point to access all the available releases.
 *
 * It is a QSortFilterProxyModel - that means the actual release data has to be provided first by the @ref ReleaseListModel .
 *
 * It's also a @ref DownloadReceiver - it tries to fetch the list of current releases when the app is started
 *
 * @property frontPage is true if the application is on the front page
 * @property beingUpdated is true when the background data update is still running (waiting for data)
 * @property filterArchitecture index of the currently selected architecture
 * @property filterText user-entered text filter
 * @property selected the currently selected release
 * @property selectedIndex the index of the currently selected release
 * @property architectures the list of the available architectures
 */
class ReleaseManager : public QSortFilterProxyModel, public DownloadReceiver {
    Q_OBJECT
    Q_PROPERTY(bool frontPage READ frontPage WRITE setFrontPage NOTIFY frontPageChanged)
    Q_PROPERTY(bool beingUpdated READ beingUpdated NOTIFY beingUpdatedChanged)

    Q_PROPERTY(int filterArchitecture READ filterArchitecture WRITE setFilterArchitecture NOTIFY filterArchitectureChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

    Q_PROPERTY(Release* selected READ selected NOTIFY selectedChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedChanged)

    Q_PROPERTY(ReleaseVariant* variant READ variant NOTIFY variantChanged)

    Q_PROPERTY(QStringList architectures READ architectures CONSTANT)
public:
    explicit ReleaseManager(QObject *parent = 0);
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    Q_INVOKABLE Release *get(int index) const;

    bool beingUpdated() const;

    bool frontPage() const;
    void setFrontPage(bool o);

    QString filterText() const;
    void setFilterText(const QString &o);

    bool updateUrl(const QString &release, const QString &version, const QString &status, const QDateTime &releaseDate, const QString &architecture, const QString &imageType, const QString &board, const QString &url, const QString &sha256, const QString &md5, int64_t size);

    QStringList architectures() const;
    int filterArchitecture() const;
    void setFilterArchitecture(int o);

    Release *selected() const;
    int selectedIndex() const;
    void setSelectedIndex(int o);

    ReleaseVariant *variant();

    // DownloadReceiver interface
    void onStringDownloaded(const QString &text) override;
    virtual void onDownloadError(const QString &message) override;

public slots:
    void fetchReleases();
    void variantChangedFilter();

signals:
    void beingUpdatedChanged();
    void frontPageChanged();
    void filterTextChanged();
    void filterArchitectureChanged();
    void selectedChanged();
    void variantChanged();

private:
    ReleaseListModel *m_sourceModel { nullptr };
    bool m_frontPage { true };
    QString m_filterText {};
    int m_filterArchitecture { 0 };
    int m_selectedIndex { 0 };
    bool m_beingUpdated { false };
    unsigned int currentDownloadingReleaseIndex = 0;

    void loadReleases(const QString &text);
};


/**
 * @brief The ReleaseListModel class
 *
 * The list model containing all available releases without filtering.
 */
class ReleaseListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ReleaseListModel(ReleaseManager *parent = 0);
    ReleaseManager *manager();

    Q_INVOKABLE Release *get(int index);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
private:
    void loadSection(const char *sectionName, const char *sectionsFilename);

    QList<Release*> m_releases {};
};


/**
 * @brief The Release class
 *
 * The class representing a fedora flavor, like for example Workstation or KDE Plasma Desktop spin.
 *
 * It can have multiple versions.
 *
 * @property index the index in the list
 * @property name the name of the release, like "Fedora Workstation"
 * @property summary the summary describing the release - displayed on the main screen
 * @property description the extensive description of the release - displayed on the detail screen
 * @property isLocal true if variant is "custom"
 * @property icon path of the icon of this release
 * @property screenshots a list of paths to screenshots (typically HTTP URLs)
 * @property prerelease true if the release contains a prerelease version of a future version
 * @property versions a list of available versions of the @ref ReleaseVersion class
 * @property versionNames a list of the names of the available versions
 * @property version the currently selected @ref ReleaseVersion
 * @property versionIndex the index of the currently selected @ref ReleaseVersion
 */
class Release : public QObject {
    Q_OBJECT
    Q_PROPERTY(int index READ index CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString summary READ summary CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)

    Q_PROPERTY(bool isLocal READ isLocal CONSTANT)

    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QStringList screenshots READ screenshots CONSTANT)

    Q_PROPERTY(QString prerelease READ prerelease NOTIFY prereleaseChanged)

    Q_PROPERTY(QQmlListProperty<ReleaseVersion> versions READ versions NOTIFY versionsChanged)
    Q_PROPERTY(QStringList versionNames READ versionNames NOTIFY versionsChanged)
    Q_PROPERTY(ReleaseVersion* version READ selectedVersion NOTIFY selectedVersionChanged)
    Q_PROPERTY(int versionIndex READ selectedVersionIndex WRITE setSelectedVersionIndex NOTIFY selectedVersionChanged)
public:
    Release(ReleaseManager *parent, int index, const QString &variant, const QString &name, const QString &summary, const QString &description, const QString &icon, const QStringList &screenshots);
    Q_INVOKABLE void setLocalFile(const QString &path);
    bool updateUrl(const QString &version, const QString &status, const QDateTime &releaseDate, const QString &architecture, const QString &imageType, const QString &board, const QString &url, const QString &sha256, const QString &md5, int64_t size);
    ReleaseManager *manager();

    int index() const;
    QString variant() const;
    QString name() const;
    QString summary() const;
    QString description() const;
    bool isLocal() const;
    QString icon() const;
    QStringList screenshots() const;
    QString prerelease() const;

    void addVersion(ReleaseVersion *version);
    void removeVersion(ReleaseVersion *version);
    QQmlListProperty<ReleaseVersion> versions();
    QList<ReleaseVersion*> versionList() const;
    QStringList versionNames() const;
    ReleaseVersion *selectedVersion() const;
    int selectedVersionIndex() const;
    void setSelectedVersionIndex(int o);

signals:
    void versionsChanged();
    void selectedVersionChanged();
    void prereleaseChanged();
private:
    int m_index { 0 };
    QString m_variant {};
    QString m_name {};
    QString m_summary {};
    QString m_description {};
    QString m_icon {};
    QStringList m_screenshots {};
    QList<ReleaseVersion *> m_versions {};
    int m_selectedVersion { 0 };
};


/**
 * @brief The ReleaseVersion class
 *
 * Represents the version of the release. It can have multiple variants (like a different architecture or netinst/live)
 *
 * @property number the version number (as string)
 * @property name the name of the release (version + alpha/beta/etc)
 * @property status the release status of the version (alpha - beta - release candidate - final)
 * @property releaseDate the release date
 * @property variants list of the version's variants, like architectures
 * @property variant the currently selected variant
 * @property variantIndex the index of the currently selected variant
 */
class ReleaseVersion : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString number READ number CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)

    Q_PROPERTY(ReleaseVersion::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QDateTime releaseDate READ releaseDate NOTIFY releaseDateChanged)

    Q_PROPERTY(QQmlListProperty<ReleaseVariant> variants READ variants NOTIFY variantsChanged)
    Q_PROPERTY(ReleaseVariant* variant READ selectedVariant NOTIFY selectedVariantChanged)
    Q_PROPERTY(int variantIndex READ selectedVariantIndex WRITE setSelectedVariantIndex NOTIFY selectedVariantChanged)

public:
    enum Status {
        FINAL,
        RELEASE_CANDIDATE,
        BETA,
        ALPHA
    };

    Q_ENUMS(Status)

    ReleaseVersion(Release *parent, const QString &number, ReleaseVersion::Status status = FINAL, QDateTime releaseDate = QDateTime());
    ReleaseVersion(Release *parent, const QString &file, int64_t size);
    Release *release();
    const Release *release() const;

    bool updateUrl(const QString &status, const QDateTime &releaseDate, const QString &architecture, const QString &imageType, const QString &board, const QString &url, const QString &sha256, const QString &md5, int64_t size);

    QString number() const;
    QString name() const;
    ReleaseVersion::Status status() const;
    QDateTime releaseDate() const;

    void addVariant(ReleaseVariant *v);
    QQmlListProperty<ReleaseVariant> variants();
    QList<ReleaseVariant*> variantList() const;
    ReleaseVariant *selectedVariant() const;
    int selectedVariantIndex() const;
    void setSelectedVariantIndex(int o);

signals:
    void variantsChanged();
    void selectedVariantChanged();
    void statusChanged();
    void releaseDateChanged();

private:
    QString m_number { "0" };
    ReleaseVersion::Status m_status { FINAL };
    QDateTime m_releaseDate {};
    QList<ReleaseVariant*> m_variants {};
    int m_selectedVariant { 0 };
};


/**
 * @brief The ReleaseVariant class
 *
 * The variant of the release version. Usually it represents different architectures. It's possible to differentiate netinst and dvd images here too.
 *
 * @property arch architecture of the variant
 * @property name the name of the release, generated from @ref arch and @ref board
 * @property board the name of supported hardware of the image
 * @property url the URL pointing to the image
 * @property shaHash SHA256 hash of the image
 * @property md5 MD5 of the image
 * @property image the path to the image on the drive
 * @property imageType the tye of the image on the drive
 * @property size the size of the image in bytes
 * @property progress the progress object of the image - reports the progress of download
 * @property status status of the variant - if it's downloading, being written, etc.
 * @property statusString string representation of the @ref status
 * @property errorString a string better describing the current error @ref status of the variant
 */
class ReleaseVariant : public QObject, public DownloadReceiver {
    Q_OBJECT
    Q_PROPERTY(ReleaseArchitecture* arch READ arch CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(ReleaseBoard* board READ board CONSTANT)

    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString shaHash READ shaHash NOTIFY shaHashChanged)
    Q_PROPERTY(QString image READ image NOTIFY imageChanged)
    Q_PROPERTY(ReleaseImageType* imageType READ imageType CONSTANT)
    Q_PROPERTY(qreal size READ size NOTIFY sizeChanged) // stored as a 64b int, UI doesn't need the precision and QML doesn't support long ints
    Q_PROPERTY(qreal realSize READ realSize NOTIFY realSizeChanged) // size after decompression
    Q_PROPERTY(Progress* progress READ progress CONSTANT)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString statusString READ statusString NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString WRITE setErrorString NOTIFY errorStringChanged)
public:
    Q_ENUMS(Type)
    enum Status {
        PREPARING = 0,
        DOWNLOADING,
        DOWNLOAD_VERIFYING,
        READY,
        WRITING_NOT_POSSIBLE,
        WRITING,
        WRITE_VERIFYING,
        FINISHED,
        FAILED_VERIFICATION,
        FAILED_DOWNLOAD,
        FAILED
    };
    Q_ENUMS(Status)
    const QStringList m_statusStrings {
        tr("Preparing"),
        tr("Downloading"),
        tr("Checking the download"),
        tr("Ready to write"),
        tr("Image file was saved to your downloads folder. Writing is not possible"),
        tr("Writing"),
        tr("Checking the written data"),
        tr("Finished!"),
        tr("The written data is corrupted"),
        tr("Download failed"),
        tr("Error")
    };

    ReleaseVariant(ReleaseVersion *parent, QString url,  QString shaHash, QString md5, int64_t size, ReleaseArchitecture *arch, ReleaseImageType *imageType, ReleaseBoard *board);
    ReleaseVariant(ReleaseVersion *parent, const QString &file, int64_t size);

    bool updateUrl(const QString &url, const QString &sha256, int64_t size);

    ReleaseVersion *releaseVersion();
    const ReleaseVersion *releaseVersion() const;
    Release *release();
    const Release *release() const;

    ReleaseArchitecture *arch() const;
    QString name() const;
    QString fullName();
    ReleaseBoard *board() const;

    QString url() const;
    QString shaHash() const;
    QString image() const;
    QString md5() const;
    ReleaseImageType *imageType() const;
    QString temporaryPath() const;
    qreal size() const;
    qreal realSize() const;
    Progress *progress();

    void setRealSize(qint64 o);

    Status status() const;
    QString statusString() const;
    void setStatus(Status s);
    QString errorString() const;
    void setErrorString(const QString &o);

    // DownloadReceiver interface
    void onFileDownloaded(const QString &path, const QString &shaHash) override;
    virtual void onDownloadError(const QString &message) override;
    void onStringDownloaded(const QString &text) override;

    static int staticOnMediaCheckAdvanced(void *data, long long offset, long long total);
    int onMediaCheckAdvanced(long long offset, long long total);

    Q_INVOKABLE bool erase();

signals:
    void imageChanged();
    void statusChanged();
    void errorStringChanged();
    void urlChanged();
    void sizeChanged();
    void realSizeChanged();
    void shaHashChanged();

public slots:
    void download();
    void resetStatus();

private:
    QString m_temporaryImage {};
    QString m_image {};
    ReleaseArchitecture *m_arch { nullptr };
    ReleaseImageType *m_image_type { nullptr };
    ReleaseBoard *m_board { nullptr };
    QString m_url {};
    QString m_shaHash {};
    QString m_md5 {};
    int64_t m_size { 0 };
    int64_t m_realSize { 0 };
    Status m_status { PREPARING };
    QString m_error {};

    Progress *m_progress { nullptr };
};

/**
 * @brief The ReleaseArchitecture class
 *
 * Class representing the possible architectures of the releases
 *
 * @property abbreviation short names for the architecture, like x86_64
 * @property description a better description what the short stands for, like Intel 64bit
 */
class ReleaseArchitecture : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList abbreviation READ abbreviation CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
public:
    enum Id {
        X86_64 = 0,
        X86,
        ARM,
        AARCH64,
        // NOTE: can't use just 'MIPSEL' because it's a predefined macro on mipsel
        MIPSEL_arch,
        RISCV64,
        E2K,
        PPC64LE,
        UNKNOWN,
        _ARCHCOUNT,
    };
    Q_ENUMS(Id);
    static ReleaseArchitecture *fromId(Id id);
    static ReleaseArchitecture *fromAbbreviation(const QString &abbr);
    static ReleaseArchitecture *fromFilename(const QString &filename);
    static bool isKnown(const QString &abbr);
    static QList<ReleaseArchitecture *> listAll();
    static QStringList listAllDescriptions();

    QStringList abbreviation() const;
    QString description() const;
    int index() const;

private:
    ReleaseArchitecture(const QStringList &abbreviation, const char *description);

    static ReleaseArchitecture m_all[];

    const QStringList m_abbreviation {};
    const char *m_description {};
    const char *m_details {};
};


/**
 * @brief The ReleaseBoard class
 *
 * Class representing the possible boards of the releases
 *
 * @property abbreviation short names for the boards, like pc or rpi4
 * @property description a better description what the short stands for, like Intel, AMD and other compatible PCs
 */
class ReleaseBoard : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList abbreviation READ abbreviation CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
public:
    enum Id {
        UNKNOWN = 0,
        NONE,
        ARM64_LIVE,
        I586_LIVE,
        X86_64_LIVE,
        TAVOLGA,
        RPI3,
        RPI4,
        POWERPC,
        ARM64,
        RISCV64QEMU,
        JETSON_NANO,
        BAIKAL_M_ITX,
        BAIKAL_M_DBM,
        MK150_02,
        HIFIVE_UNLEASHED,
        _BOARDCOUNT,
    };
    Q_ENUMS(Id);
    static ReleaseBoard *fromId(Id id);
    static ReleaseBoard *fromAbbreviation(const QString &abbr);
    static bool isKnown(const QString &abbr);
    static QList<ReleaseBoard *> listAll();
    static QStringList listAllDescriptions();

    QStringList abbreviation() const;
    QString description() const;
    int index() const;

private:
    ReleaseBoard(const QStringList &abbreviation, const char *description);

    static ReleaseBoard m_all[];

    const QStringList m_abbreviation {};
    const char *m_description {};
    const char *m_details {};
};


/**
 * @brief The ReleaseImageType class
 *
 * Class representing the possible image types of the releases
 *
 * @property abbreviation short names for the type, like iso
 * @property name a common name what the short stands for, like "ISO DVD"
 * @property description a better description what the short stands for, like "ISO format image"
 */
class ReleaseImageType : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList abbreviation READ abbreviation CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool supportedForWriting READ supportedForWriting CONSTANT)
    Q_PROPERTY(bool canWriteWithRootfs READ canWriteWithRootfs CONSTANT)
public:
    enum Id {
        ISO = 0,
        IMG_XZ,
        TAR_XZ,
        RECOVERY_TAR,
        _IMAGETYPECOUNT,
    };
    Q_ENUMS(Id);
    static ReleaseImageType *fromId(Id id);
    static ReleaseImageType *fromAbbreviation(const QString &abbr);
    static ReleaseImageType *fromFilename(const QString &filename);
    static bool isKnown(const QString &abbr);
    static QList<ReleaseImageType *> listAll();
    static QStringList listAllNames();

    QStringList abbreviation() const;
    QString name() const;
    QString description() const;
    bool supportedForWriting() const;
    bool canWriteWithRootfs() const;
    bool canMD5checkAfterWrite() const;
    int index() const;

private:
    ReleaseImageType(const QStringList &abbreviation, const char *name, const char *description);

    static ReleaseImageType m_all[];

    const QStringList m_abbreviation {};
    const char *m_name {};
    const char *m_description {};
};

#endif // RELEASEMANAGER_H
