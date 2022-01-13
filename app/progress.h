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

#ifndef PROGRESS_H
#define PROGRESS_H

#include <QObject>

/**
 * @brief The Progress class
 *
 * Reports the ratio progress of some activity
 *
 * @property ratio in the range [0.0, 1.0]
 * @property leftSize how much size is left until completion 
 */
class Progress : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal ratio READ ratio NOTIFY ratioChanged)
    Q_PROPERTY(qreal leftSize READ leftSize NOTIFY leftSizeChanged)

public:
    using QObject::QObject;

    qreal ratio() const;
    qreal leftSize() const;

    void setCurrent(const qreal newCurrent);
    void setMax(const qreal newMax);

signals:
    void ratioChanged();
    void leftSizeChanged();

private:
    qreal m_current;
    qreal m_max;
};

#endif // PROGRESS_H
