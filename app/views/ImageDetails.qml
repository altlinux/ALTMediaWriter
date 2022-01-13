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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0

import MediaWriter 1.0

import "../simple"
import "../complex"

Item {
    id: root
    anchors.fill: parent

    property bool focused: contentList.currentIndex === 1
    enabled: focused

    function toMainScreen() {
        otherVariantsPopover.open = false
        canGoBack = false
        contentList.currentIndex--
    }

    signal stepForward

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        onClicked:
            if (mouse.button == Qt.BackButton) {
                toMainScreen()
            }
    }

    ScrollView {
        activeFocusOnTab: false
        focus: true
        anchors {
            fill: parent
            leftMargin: anchors.rightMargin
        }
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        flickableItem.flickableDirection: Flickable.VerticalFlick
        contentItem: Item {
            x: mainWindow.margin
            width: root.width - 2 * mainWindow.margin
            height: childrenRect.height + 64 + 32

            ColumnLayout {
                y: 18
                width: parent.width
                spacing: 24

                RowLayout {
                    id: tools
                    Layout.fillWidth: true
                    BackButton {
                        id: backButton
                        onClicked: toMainScreen()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    AdwaitaButton {
                        text: qsTr("Create Live USB…")
                        color: "#628fcf"
                        textColor: "white"
                        onClicked: {
                            if (dlDialog.visible) {
                                return
                            }
                            deviceNotification.open = false
                            otherVariantsPopover.open = false
                            dlDialog.visible = true
                            releases.selected.variant.download()
                        }
                        enabled: !releases.selected.isCustom || (releases.selected.variant.status === Variant.READY_FOR_WRITING)
                    }
                }

                RowLayout {
                    z: 1 // so the popover stays over the text below
                    spacing: 24
                    Item {
                        Layout.preferredWidth: 64 + 16
                        Layout.preferredHeight: 64
                        IndicatedImage {
                            x: 12
                            source: releases.selected.icon ? releases.selected.icon: ""
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: parent.width
                            sourceSize.height: parent.height
                        }
                    }
                    ColumnLayout {
                        Layout.fillHeight: true
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment : Qt.AlignLeft
                                font.pointSize: 14
                                text: releases.selected.displayName
                                color: palette.windowText
                            }
                        }
                        ColumnLayout {
                            width: parent.width
                            spacing: 6
                            opacity: releases.selected.isCustom ? 0.0 : 1.0
                            Text {
                                font.pointSize: 10
                                color: mixColors(palette.window, palette.windowText, 0.3)
                                visible: releases.selected.variant
                                text: releases.selected.variant.name
                            }
                            Text {
                                font.pointSize: 8
                                color: mixColors(palette.window, palette.windowText, 0.3)
                                visible: releases.selected.variant
                                text: releases.selected.variant && releases.selected.variant.fileTypeName
                            }
                            RowLayout {
                                spacing: 0
                                width: parent.width
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    visible: releases.selected.variants.length > 1
                                    text: qsTr("Other variants...")
                                    font.pointSize: 8
                                    color: otherVariantsMouseArea.containsPress ? Qt.lighter("#1d61bf", 1.7) : otherVariantsMouseArea.containsMouse ? Qt.darker("#1d61bf", 1.5) : "#1d61bf"
                                    Behavior on color { ColorAnimation { duration: 100 } }
                                    MouseArea {
                                        id: otherVariantsMouseArea
                                        activeFocusOnTab: parent.visible
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        function action() {
                                            otherVariantsPopover.open = !otherVariantsPopover.open
                                        }
                                        Keys.onSpacePressed: action()
                                        onClicked: {
                                            action()
                                        }
                                        FocusRectangle {
                                            anchors.fill: parent
                                            anchors.margins: -2
                                            visible: parent.activeFocus
                                        }
                                    }

                                    Rectangle {
                                        anchors {
                                            left: parent.left
                                            right: parent.right
                                            top: parent.bottom
                                        }
                                        radius: height / 2
                                        color: parent.color
                                        height: 1
                                    }

                                    AdwaitaPopOver {
                                        id: otherVariantsPopover
                                        z: 2
                                        anchors {
                                            horizontalCenter: parent.horizontalCenter
                                            top: parent.bottom
                                            topMargin: 8 + opacity * 24
                                        }

                                        ColumnLayout {
                                            spacing: 9
                                            ExclusiveGroup {
                                                id: otherVariantsExclusiveGroup
                                            }
                                            Repeater {
                                                model: releases.selected.variants
                                                AdwaitaRadioButton {
                                                    text: name
                                                    Layout.alignment: Qt.AlignVCenter
                                                    exclusiveGroup: otherVariantsExclusiveGroup
                                                    checked: index == releases.selected.variantIndex
                                                    onCheckedChanged: {
                                                        if (checked) {
                                                            releases.selected.variantIndex = index
                                                        }
                                                        otherVariantsPopover.open = false
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    width: Layout.width
                    wrapMode: Text.WordWrap
                    text: releases.selected.description
                    textFormat: Text.RichText
                    font.pointSize: 9
                    color: palette.windowText
                }
                Repeater {
                    id: screenshotRepeater
                    model: releases.selected.screenshots
                    ZoomableImage {
                        z: 0
                        smooth: true
                        cache: false
                        Layout.fillWidth: true
                        Layout.preferredHeight: width / sourceSize.width * sourceSize.height
                        fillMode: Image.PreserveAspectFit
                        source: modelData
                    }
                }
            }
        }
        style: ScrollViewStyle {
            incrementControl: Item {}
            decrementControl: Item {}
            corner: Item {
                implicitWidth: 11
                implicitHeight: 11
            }
            scrollBarBackground: Rectangle {
                color: Qt.darker(palette.window, 1.2)
                implicitWidth: 11
                implicitHeight: 11
            }
            handle: Rectangle {
                color: mixColors(palette.window, palette.windowText, 0.5)
                x: 3
                y: 3
                implicitWidth: 6
                implicitHeight: 7
                radius: 4
            }
            transientScrollBars: false
            handleOverlap: 1
            minimumHandleLength: 10
        }
    }
}
