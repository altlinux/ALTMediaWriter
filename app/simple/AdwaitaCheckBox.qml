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

CheckBox {
    id: root

    indicator: AdwaitaRectangle {
        implicitWidth: 15
        implicitHeight: 15
        Item {
            visible: root.checked
            rotation: -45
            anchors.fill: parent
            Rectangle {
                color: palette.text
                x: 5
                y: 4
                width: 3
                height: 6
                radius: 4
            }
            Rectangle {
                color: palette.text
                x: 5
                y: 8
                width: 12
                height: 3
                radius: 4
            }
        }
    }
    contentItem: Text {
        font.pointSize: 9
        text: root.text
        color: root.enabled ? palette.windowText : disabledPalette.windowText
    }
    spacing: 8
}

