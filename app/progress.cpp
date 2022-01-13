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

#include "progress.h"

qreal Progress::ratio() const {
    return (m_current / m_max);
}

qreal Progress::leftSize() const {
    return (m_max - m_current);
}

void Progress::setCurrent(const qreal newCurrent) {
    if (m_current != newCurrent) {
        m_current = newCurrent;

        emit ratioChanged();
        emit leftSizeChanged();
    }
}

void Progress::setMax(const qreal newMax) {
    if (m_max != newMax) {
        m_max = newMax;

        emit ratioChanged();
        emit leftSizeChanged();
    }
}
