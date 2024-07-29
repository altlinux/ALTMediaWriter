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

import QtQuick 2.3
import QtQuick.Controls 2.15
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

import MediaWriter 1.0

import "../simple"
import "../complex"

Dialog {
    id: dialog
    title: qsTr("Write %1").arg(releases.selected.displayName)

    height: layout.height + 36
    standardButtons: StandardButton.NoButton

    width: 640

    function reset() {
        writeArrow.color = palette.text
    }

    onVisibleChanged: {
        // When dialog is closed via Cancel button or Close button, cancel current download or write
        if (!visible) {
            if (drives.selected) {
                drives.selected.cancel()
            }
            releases.selected.variant.cancelDownload()
        }
        releases.selected.variant.resetStatus()
        reset()
    }

    contentItem: Rectangle {
        id: dialogContainer
        anchors.fill: parent
        color: palette.window
        focus: true

        states: [
            State {
                name: "preparing"
                when: releases.selected.variant.status === Variant.PREPARING
                PropertyChanges {
                    target: progressBar;
                    value: 0.0/0.0
                }
            },
            State {
                name: "downloading"
                when: releases.selected.variant.status === Variant.DOWNLOADING
                PropertyChanges {
                    target: messageDownload
                    visible: true
                }
                PropertyChanges {
                    target: progressBar;
                    value: releases.selected.variant.progress.ratio
                }
            },
            State {
                name: "resuming"
                when: releases.selected.variant.status === Variant.DOWNLOAD_RESUMING
                PropertyChanges {
                    target: progressBar;
                    value: 0.0/0.0
                }
            },
            State {
                name: "download_verifying"
                when: releases.selected.variant.status === Variant.DOWNLOAD_VERIFYING
                PropertyChanges {
                    target: messageDownload
                    visible: true
                }
                PropertyChanges {
                    target: progressBar;
                    value: releases.selected.variant.progress.ratio;
                    progressColor: Qt.lighter("green")
                }
            },
            State {
                name: "ready_no_drives"
                when: releases.selected.variant.status === Variant.READY_FOR_WRITING && drives.length <= 0
            },
            State {
                name: "ready"
                when: releases.selected.variant.status === Variant.READY_FOR_WRITING && drives.length > 0
                PropertyChanges {
                    target: messageLoseData;
                    visible: true
                }
                PropertyChanges {
                    target: rightButton;
                    enabled: releases.selected.variant.canWrite;
                    color: "red";
                    onClicked: drives.selected.write(releases.selected.variant)
                }
            },
            State {
                name: "writing_not_possible"
                when: releases.selected.variant.status === Variant.WRITING_NOT_POSSIBLE
                PropertyChanges {
                    target: driveCombo;
                    enabled: false;
                    placeholderText: qsTr("Writing is not possible")
                }
            },
            State {
                name: "writing"
                when: releases.selected.variant.status === Variant.WRITING
                PropertyChanges {
                    target: messageDriveSize
                    enabled: false
                }
                PropertyChanges {
                    target: messageRestore;
                    visible: true
                }
                PropertyChanges {
                    target: driveCombo;
                    enabled: false
                }
                PropertyChanges {
                    target: progressBar;
                    value: drives.selected.progress.ratio;
                    progressColor: "red"
                }
            },
            State {
                name: "write_verifying"
                when: releases.selected.variant.status === Variant.WRITE_VERIFYING
                PropertyChanges {
                    target: messageDriveSize
                    enabled: false
                }
                PropertyChanges {
                    target: messageRestore;
                    visible: true
                }
                PropertyChanges {
                    target: driveCombo;
                    enabled: false
                }
                PropertyChanges {
                    target: progressBar;
                    value: drives.selected.progress.ratio;
                    progressColor: Qt.lighter("green")
                }
            },
            State {
                name: "finished"
                when: releases.selected.variant.status === Variant.WRITING_FINISHED
                PropertyChanges {
                    target: messageDriveSize
                    enabled: false
                }
                PropertyChanges {
                    target: messageRestore;
                    visible: true
                }
                PropertyChanges {
                    target: leftButton;
                    text: qsTr("Close");
                    color: "#628fcf";
                    textColor: "white"
                    onClicked: {
                        dialog.close()
                    }
                }
            },
            State {
                name: "write_verifying_failed_no_drives"
                when: releases.selected.variant.status === Variant.WRITE_VERIFYING_FAILED && drives.length <= 0
                PropertyChanges {
                    target: rightButton;
                    text: qsTr("Retry");
                    enabled: false;
                    color: "red";
                    onClicked: drives.selected.write(releases.selected.variant);
                }
            },
            State {
                name: "write_verifying_failed"
                when: releases.selected.variant.status === Variant.WRITE_VERIFYING_FAILED && drives.length > 0
                PropertyChanges {
                    target: messageLoseData;
                    visible: true;
                }
                PropertyChanges {
                    target: rightButton;
                    text: qsTr("Retry");
                    enabled: true;
                    color: "red";
                    onClicked: drives.selected.write(releases.selected.variant);
                }
            },
            State {
                name: "failed_download"
                when: releases.selected.variant.status === Variant.DOWNLOAD_FAILED
                PropertyChanges {
                    target: driveCombo;
                    enabled: false
                }
                PropertyChanges {
                    target: rightButton;
                    text: qsTr("Retry");
                    enabled: true;
                    color: "#628fcf";
                    onClicked: releases.selected.variant.download()
                }
            },
            State {
                name: "failed_no_drives"
                when: releases.selected.variant.status === Variant.WRITING_FAILED && drives.length <= 0
                PropertyChanges {
                    target: rightButton;
                    text: qsTr("Retry");
                    enabled: false;
                    color: "red";
                    onClicked: drives.selected.write(releases.selected.variant)
                }
            },
            State {
                name: "failed"
                when: releases.selected.variant.status === Variant.WRITING_FAILED && drives.length > 0
                PropertyChanges {
                    target: messageLoseData;
                    visible: true
                }
                PropertyChanges {
                    target: rightButton;
                    text: qsTr("Retry");
                    enabled: true;
                    color: "red";
                    onClicked: drives.selected.write(releases.selected.variant)
                }
            }
        ]

        Keys.onEscapePressed: {
            if ([Variant.WRITING, Variant.WRITE_VERIFYING].indexOf(releases.selected.variant.status) < 0) {
                dialog.visible = false
            }
        }

        ScrollView {
            id: contentScrollView
            anchors.fill: parent
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            activeFocusOnTab: false

            contentItem: Item {
                width: contentScrollView.width - 18
                height: layout.height + 18
                ColumnLayout {
                    id: layout
                    spacing: 18
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                        topMargin: 18
                        leftMargin: 18
                        rightMargin: 18
                    }
                    ColumnLayout {
                        id: infoColumn
                        spacing: 4
                        Layout.fillWidth: true

                        InfoMessage {
                            id: messageDownload
                            visible: false
                            width: infoColumn.width
                            text: qsTr("The file will be saved to your Downloads folder.")
                        }

                        InfoMessage {
                            id: messageLoseData
                            visible: false
                            width: infoColumn.width
                            text: qsTr("By writing, you will lose all of the data on %1.").arg(driveCombo.currentText)
                        }

                        InfoMessage {
                            id: messageRestore
                            visible: false
                            width: infoColumn.width
                            text: qsTr("Your drive will be resized to a smaller capacity. You may resize it back to normal by using ALT Media Writer; this will remove installation media from your drive.")
                        }

                        InfoMessage {
                            id: messageSelectedImage
                            width: infoColumn.width
                            text: "<font color=\"gray\">" + qsTr("Selected:") + "</font> " + releases.selected.variant.fileName
                        }

                        InfoMessage {
                            id: messageDriveSize
                            width: infoColumn.width
                            enabled: true
                            visible: enabled && drives.selected && drives.selected.size > 160 * 1024 * 1024 * 1024 // warn when it's more than 160GB
                            text: qsTr("The selected drive's size is %1. It's possible you have selected an external drive by accident!").arg(drives.selected ? drives.selected.readableSize : "N/A")
                        }

                        InfoMessage {
                            error: true
                            width: infoColumn.width
                            visible: releases.selected.variant && releases.selected.variant.errorString.length > 0
                            text: releases.selected.variant ? releases.selected.variant.errorString : ""
                        }
                    }

                    ColumnLayout {
                        width: parent.width
                        spacing: 5

                        Behavior on y {
                            NumberAnimation {
                                duration: 1000
                            }
                        }
                        
                        Text {
                            visible: true
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            horizontalAlignment: Text.AlignHCenter
                            font.pointSize: 9
                            property double leftSize: releases.selected.variant.progress.leftSize
                            property string leftStr:  leftSize <= 0                    ? "" :
                                                     (leftSize < 1024)                 ? qsTr("(%1 B left)").arg(leftSize) :
                                                     (leftSize < (1024 * 1024))        ? qsTr("(%1 KB left)").arg((leftSize / 1024).toFixed(1)) :
                                                     (leftSize < (1024 * 1024 * 1024)) ? qsTr("(%1 MB left)").arg((leftSize / 1024 / 1024).toFixed(1)) :
                                                                                         qsTr("(%1 GB left)").arg((leftSize / 1024 / 1024 / 1024).toFixed(1))
                            text: releases.selected.variant.statusString + (releases.selected.variant.status == Variant.DOWNLOADING ? (" " + leftStr) : "")
                            color: palette.windowText
                        }
                        Item {
                            Layout.fillWidth: true
                            height: childrenRect.height
                            AdwaitaProgressBar {
                                id: progressBar
                                width: parent.width
                                progressColor: "#54aada"
                                value: 0.0
                            }
                        }
                        // TODO: need to uncheck when drive
                        // changes because delayedwrite
                        // starts a helper that is setup for
                        // one specific drive
                        AdwaitaCheckBox {
                            id: delayedWriteCheck
                            text: qsTr("Write the image after downloading")
                            enabled: drives.selected && ((releases.selected.variant.status == Variant.DOWNLOADING) || (releases.selected.variant.status == Variant.DOWNLOAD_RESUMING)) && releases.selected.variant.canWrite
                            visible: enabled

                            onCheckedChanged: {
                                releases.selected.variant.setDelayedWrite(checked)
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment : Qt.AlignHCenter
                        spacing: 32
                        Image {
                            source: releases.selected.icon
                            Layout.preferredWidth: 64
                            Layout.preferredHeight: 64
                            sourceSize.width: 64
                            sourceSize.height: 64
                            fillMode: Image.PreserveAspectFit
                        }
                        Arrow {
                            id: writeArrow
                            Layout.alignment : Qt.AlignVCenter
                            scale: 1.4
                            SequentialAnimation {
                                running: releases.selected.variant.status == Variant.WRITING
                                loops: -1
                                onStopped: {
                                    if (releases.selected.variant.status == Variant.WRITING_FINISHED) { {
                                        writeArrow.color = "#00dd00"
                                    }
                                    } else {
                                        writeArrow.color = palette.text
                                    }
                                }
                                ColorAnimation {
                                    duration: 3500
                                    target: writeArrow
                                    property: "color"
                                    to: "red"
                                }
                                PauseAnimation {
                                    duration: 500
                                }
                                ColorAnimation {
                                    duration: 3500
                                    target: writeArrow
                                    property: "color"
                                    to: palette.text
                                }
                                PauseAnimation {
                                    duration: 500
                                }
                            }
                        }
                        ComboBox {
                            id: driveCombo
                            Layout.fillWidth: true
                            model: drives
                            textRole: "display"
                            Binding on currentIndex {
                                when: drives
                                value: drives.selectedIndex
                            }
                            onActivated: {
                                if ([Variant.WRITING_FINISHED, Variant.WRITING_FAILED].indexOf(releases.selected.variant.status) >= 0) {
                                    releases.selected.variant.resetStatus()
                                }
                            }
                            onCurrentIndexChanged: {
                                drives.setSelectedIndex(driveCombo.currentIndex)
                            }
                            displayText: currentIndex === -1 || !currentText ? qsTr("There are no portable drives connected") : currentText
                        }
                    }

                    ColumnLayout {
                        z: -1
                        Layout.maximumWidth: parent.width
                        spacing: 3
                        Item {
                            height: 3
                            width: 1
                        }
                        Text {
                            visible: !releases.selected.variant.canWrite
                            font.pointSize: 10
                            Layout.fillWidth: true
                            width: Layout.width
                            wrapMode: Text.WordWrap
                            text: qsTr("Writing this image type is not supported.")
                            color: "red"
                        }
                        Text {
                            visible: releases.selected.variant.noMd5sum
                            font.pointSize: 10
                            Layout.fillWidth: true
                            width: Layout.width
                            wrapMode: Text.WordWrap
                            text: qsTr("This image won't be verified after writing because no MD5 sum was found.")
                        }
                        Text {
                            visible: releases.selected.variant.isCompressed
                            font.pointSize: 10
                            Layout.fillWidth: true
                            width: Layout.width
                            wrapMode: Text.WordWrap
                            text: qsTr("This image won't be verified after writing because it is a compressed image.")
                        }
                        RowLayout {
                            height: rightButton.height
                            Layout.minimumWidth: parent.width
                            Layout.maximumWidth: parent.width
                            spacing: 6

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            AdwaitaButton {
                                id: leftButton
                                Layout.alignment: Qt.AlignRight
                                Behavior on implicitWidth { NumberAnimation { duration: 80 } }
                                text: qsTr("Cancel")
                                enabled: true
                                onClicked: {
                                    delayedWriteCheck.checked = false
                                    dialog.close()
                                }
                            }
                            AdwaitaButton {
                                id: rightButton
                                Layout.alignment: Qt.AlignRight
                                Behavior on implicitWidth { NumberAnimation { duration: 80 } }
                                textColor: enabled ? "white" : palette.text
                                text: qsTr("Write to Disk")
                                enabled: false
                            }
                        }
                    }
                }
            }
        }
    }
}
