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

import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1

import "../simple"
import "../complex"

FocusScope {
    id: imageList
    
    readonly property int searchBarHeight: 36
    property alias currentIndex: listView.currentIndex
    property int lastIndex: -1

    property bool focused: contentList.currentIndex === 0
    signal stepForward(int index)
    onStepForward: lastIndex = index
    enabled: focused

    anchors.fill: parent
    clip: true

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton
        onClicked: {
            if (lastIndex >= 0 && mouse.button == Qt.ForwardButton) {
                stepForward(lastIndex)
            }
        }
    }

    Item {
        anchors {
            top: parent.top
            topMargin: 12
            left: parent.left
            right: parent.right
            leftMargin: mainWindow.margin
            rightMargin: mainWindow.margin
        }

        width: listView.width
        height: searchBarHeight
        z: 2
        enabled: !releases.filter.frontPage
        visible: enabled
        opacity: enabled ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }

        // Blank rectangle to hide scrolling listview behind header
        Rectangle {
            anchors.fill: parent
            color: palette.window
        }

        Rectangle {
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                right: archSelect.left
                rightMargin: 4
            }
            border {
                color: searchInput.activeFocus ? "#4a90d9" : Qt.darker(palette.button, 1.3)
                width: 1
            }
            radius: 5
            color: palette.background
            z: 1

            Item {
                id: magnifyingGlass
                anchors {
                    left: parent.left
                    leftMargin: (parent.height - height) / 2
                    verticalCenter: parent.verticalCenter
                }
                height: childrenRect.height + 3
                width: childrenRect.width + 2

                Rectangle {
                    height: 11
                    antialiasing: true
                    width: height
                    radius: height / 2
                    color: palette.text
                    Rectangle {
                        height: 7
                        antialiasing: true
                        width: height
                        radius: height / 2
                        color: palette.background
                        anchors.centerIn: parent
                    }
                    Rectangle {
                        height: 2
                        width: 6
                        radius: 2
                        x: 8
                        y: 11
                        rotation: 45
                        color: palette.text
                    }
                }
            }

            TextInput {
                id: searchInput
                anchors {
                    left: magnifyingGlass.right
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                    margins: 8
                }
                activeFocusOnTab: true
                Text {
                    anchors.fill: parent
                    color: "light gray"
                    font.pointSize: 9
                    text: qsTr("Find an operating system image")
                    visible: !parent.activeFocus && parent.text.length == 0
                    verticalAlignment: Text.AlignVCenter
                }
                verticalAlignment: TextInput.AlignVCenter
                onTextChanged: releases.filter.setFilterText(text)
                clip: true
                color: palette.text
            }
        }

        AdwaitaComboBox {
            id: archSelect
            anchors {
                right: parent.right
            }
            width: 148
            activeFocusOnTab: visible
            model: releases.architectures
            onCurrentIndexChanged:  {
                releases.filter.setFilterArch(currentIndex)
            }
        }
    }

    Component {
        id: listExpander

        Column {
            Rectangle {
                id: frontFooter

                
                clip: true
                activeFocusOnTab: true
                radius: 3
                color: palette.window
                width: listView.width
                height: 32
                z: -1

                // Disable and hide footer until releases are loaded and then fade it in.
                enabled: !releases.downloadingMetadata
                opacity: releases.downloadingMetadata ? 0.0 : 1.0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 600
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.topMargin: -10
                    color: threeDotMouse.containsPress ? Qt.darker(palette.window, 1.2) : threeDotMouse.containsMouse ? palette.window : palette.background
                    Behavior on color { ColorAnimation { duration: 120 } }
                    radius: 5
                    border {
                        color: Qt.darker(palette.background, 1.3)
                        width: 1
                    }
                }

                Column {
                    id: threeDotDots
                    property bool hidden: false
                    opacity: hidden ? 0.0 : 1.0
                    Behavior on opacity { NumberAnimation { duration: 60 } }
                    anchors.centerIn: parent
                    spacing: 3
                    Repeater {
                        model: 3
                        Rectangle { height: 4; width: 4; radius: 1; color: mixColors(palette.windowText, palette.window, 0.75); antialiasing: true }
                    }
                }

                Text {
                    id: threeDotText
                    y: threeDotDots.hidden ? parent.height / 2 - height / 2 : -height
                    font.pointSize: 9
                    anchors.horizontalCenter: threeDotDots.horizontalCenter
                    Behavior on y { NumberAnimation { duration: 60 } }
                    clip: true
                    text: qsTr("Display additional ALT flavors")
                    color: "gray"
                }

                FocusRectangle {
                    visible: frontFooter.activeFocus
                    anchors.fill: parent
                    anchors.margins: 2
                }

                Timer {
                    id: threeDotTimer
                    interval: 200
                    onTriggered: {
                        threeDotDots.hidden = true
                    }
                }

                Keys.onSpacePressed: threeDotMouse.action()
                MouseArea {
                    id: threeDotMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onHoveredChanged: {
                        if (containsMouse && !pressed) {
                            threeDotTimer.start()
                        }
                        if (!containsMouse) {
                            threeDotTimer.stop()
                            threeDotDots.hidden = false
                        }
                    }
                    function action() {
                        releases.filter.leaveFrontPage()
                        listView.state = "full"
                    }
                    onClicked: {
                        action()
                    }
                }
            }
        }
    }

    Component {
        id: aboutFooter

        Item {
            height: aboutColumn.height + 72
            width: listView.width

            Column {
                id: aboutColumn
                width: parent.width
                spacing: 0
                Item {
                    width: parent.width
                    height: 64

                    Text {
                        text: qsTr("About ALT Media Writer")
                        font.pointSize: 9
                        color: palette.windowText
                        anchors {
                            bottom: parent.bottom
                            left: parent.left
                            leftMargin: 18
                            bottomMargin: 12
                        }
                    }
                }
                Rectangle {
                    width: parent.width
                    radius: 5
                    color: palette.background
                    border {
                        color: Qt.darker(palette.background, 1.3)
                        width: 1
                    }
                    height: childrenRect.height + 24
                    Behavior on height { NumberAnimation {} }
                    Column {
                        id: aboutLayout
                        spacing: 3
                        y: 12
                        x: 32
                        width: parent.width
                        move: Transition { NumberAnimation { properties: "y" } }

                        Text {
                            width: parent.width
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            text: qsTr("Version %1").arg(mediawriterVersion)
                            textFormat: Text.RichText
                            font.pointSize: 9
                            color: palette.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            text: qsTr("Please report bugs or your suggestions on %1").arg("<a href=\"https://github.com/altlinux/MediaWriter/issues\">https://github.com/altlinux/MediaWriter/</a>")
                            textFormat: Text.RichText
                            font.pointSize: 9
                            onLinkActivated: Qt.openUrlExternally(link)
                            color: Qt.darker("light gray")
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                            }
                        }
                    }
                }
            }
        }
    }
            
    ScrollView {
        id: scrollView
        anchors.fill: parent

        ListView {
            id: listView
            clip: true
            focus: true
            anchors {
                topMargin: mainWindow.margin
                leftMargin: mainWindow.margin
                // NOTE: when leaving front page and scrollView gets a scrollbar, the width is reduced, so recalculate right margin so that the right side of listView doesn't move due to that
                rightMargin: anchors.leftMargin - (scrollView.width - scrollView.viewport.width)
            }

            model: releases.filter
            footer: listExpander

            // When exiting front page, move the list to the top of the screen
            states: State {
                name: "full"

                PropertyChanges {
                    target: listView

                    // Move the list up so it occupies all available vertical space
                    anchors.topMargin: 12 + searchBarHeight
                    // Replace footer
                    footer: aboutFooter
                }
            }
            transitions: Transition {
                to: "full"
                PropertyAnimation {
                    properties: "topMargin"
                    target: listView
                    duration: 200
                }
            }

            delegate: DelegateImage {
                focus: true
            }

            remove: Transition {
                NumberAnimation { properties: "x"; to: width; duration: 300 }
            }
            removeDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 300 }
            }
            add: Transition {
                NumberAnimation { properties: releases.filter.frontPage ? "y" : "x"; from: releases.filter.frontPage ? 0 : -width; duration: 300 }
            }
            addDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 300 }
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
            handleOverlap: -2
            minimumHandleLength: 10
        }
    }

    Row {
        anchors {
            bottom: parent.bottom
            bottomMargin: mainWindow.margin / 2
            right: parent.right
            rightMargin: mainWindow.margin + 5
        }
        opacity: releases.downloadingMetadata ? 0.8 : 0.0
        visible: releases.filter.frontPage && (opacity > 0.01)
        z: 1
        spacing: 3
        Behavior on opacity { NumberAnimation { } }

        BusyIndicator {
            anchors.verticalCenter: parent.verticalCenter
            height: downloadingNotification.height * 0.8
            width: height
        }
        Text {
            id: downloadingNotification
            text: qsTr("Downloading releases info")
            font.pointSize: 9
            color: "#7a7a7a"
        }
    }
}
