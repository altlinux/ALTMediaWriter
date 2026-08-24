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
import QtQuick.Layouts
import MediaWriter 1.0

Rectangle {
    id: root
    clip: true
    enabled: open
    focus: open

    color: "#729FCF"
    border {
        color: "#4C7BB2"
        width: 1
    }

    height: units.gridUnit * 4
    property real margin: open ? 0 : -(units.gridUnit * 4)
    anchors.topMargin: margin
    Behavior on margin {
        NumberAnimation {
            duration: 300
            easing.type: Easing.OutElastic
            easing.amplitude: 0.7
            easing.period: 0.4
        }
    }

    property bool open: false

    property alias text: label.text
    property alias acceptText: buttonAccept.text

    signal accepted
    signal cancelled

    Keys.onEscapePressed: open = false

    RowLayout {
        anchors {
            fill: parent
            margins: units.gridUnit
        }
        spacing: 2 * units.largeSpacing
        Text {
            id: label

            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere

            font.pointSize: 9
            color: "white"
            textFormat: Text.RichText
        }

        AdwaitaButton {
            id: buttonAccept
            visible: text.length

            onClicked: {
                root.open = false
                root.accepted()
            }

            Layout.alignment: Qt.AlignVCenter
        }

        AdwaitaButton {
            id: buttonCancel
            isFlat: true
            color: "transparent"
            implicitWidth: 20
            implicitHeight: 20
            Cross {
                color: palette.windowText
                anchors.centerIn: parent
                width: 8
                height: 8
            }

            onClicked: {
                root.open = false
                root.cancelled()
            }

            Layout.alignment: Qt.AlignVCenter
        }
    }
}

