
#include "file_type.h"

#include <QObject>

const QList<FileType> file_type_all = []() {
    QList<FileType> out;

    for (int i = 0; i < FileType_COUNT; i++) {
        const FileType architecture = (FileType) i;
        out.append(architecture);
    }

    return out;
}();

QStringList file_type_strings(const FileType file_type) {
    switch (file_type) {
        case FileType_ISO: return {"iso", "dvd"};
        case FileType_TAR: return {"tar"};
        case FileType_TAR_GZ: return {"tgz", "tar.gz"};
        case FileType_TAR_XZ: return {"archive", "tar.xz"};
        case FileType_IMG: return {"img"};
        case FileType_IMG_GZ: return {"igz", "img.gz"};
        case FileType_IMG_XZ: return {"ixz", "img.xz"};
        case FileType_RECOVERY_TAR: return {"trc", "recovery.tar"};
        case FileType_UNKNOWN: return {};
        case FileType_COUNT: return {};
    }
    return QStringList();
}

QString file_type_name(const FileType file_type) {
    switch (file_type) {
        case FileType_ISO: return QObject::tr("ISO DVD");
        case FileType_TAR: return QObject::tr("TAR Archive");
        case FileType_TAR_GZ: return QObject::tr("GZIP TAR Archive");
        case FileType_TAR_XZ: return QObject::tr("LZMA TAR Archive");
        case FileType_IMG: return QObject::tr("IMG");
        case FileType_IMG_GZ: return QObject::tr("GZIP IMG");
        case FileType_IMG_XZ: return QObject::tr("LZMA IMG");
        case FileType_RECOVERY_TAR: return QObject::tr("Recovery TAR Archive");
        case FileType_UNKNOWN: return QObject::tr("Unknown");
        case FileType_COUNT: return QString();
    }
    return QString();
}

FileType file_type_from_filename(const QString &filename) {
    FileType matching_type = FileType_UNKNOWN;
    QString matching_string = QString();

    for (const FileType &type : file_type_all) {
        const QStringList strings = file_type_strings(type);

        for (const QString &string : strings) {
            // NOTE: need to select the longest string for cases like ".tar" and "recovery.tar"
            const bool string_matches = filename.endsWith(string, Qt::CaseInsensitive);
            const bool this_string_is_longer = (string.length() > matching_string.length());

            if (string_matches && this_string_is_longer) {
                matching_type = type;
                matching_string = string;
            }
        }
    }

    return matching_type;
}

bool file_type_can_write(const FileType file_type) {
    static const QList<FileType> supported_file_types = {
        FileType_ISO,
        FileType_IMG,
        FileType_IMG_XZ,
    };

    return supported_file_types.contains(file_type);
}
