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

import QtQuick
import QtQuick.Controls

RadioButton {
    id: root

    implicitWidth: Math.max(indicator.implicitWidth + spacing + contentItem.implicitWidth, 0)
    implicitHeight: Math.max(indicator.implicitHeight, contentItem.implicitHeight)

    focusPolicy: Qt.NoFocus

    indicator: AdwaitaRectangle {
        implicitWidth: 16
        implicitHeight: 16
        radius: width / 2 + 1

        Rectangle {
            anchors.centerIn: parent
            width: parent.width / 3
            height: parent.width / 3
            radius: parent.radius / 3
            color: palette.text
            visible: root.checked
        }
    }

    contentItem: Text {
        id: text
        font.pointSize: 9
        text: root.text
        color: palette.windowText
        verticalAlignment: Text.AlignVCenter

        anchors {
            left: root.indicator.right
            leftMargin: root.spacing
            verticalCenter: root.indicator.verticalCenter
        }
    }
    spacing: 6
}

