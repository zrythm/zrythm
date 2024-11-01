// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: control

    required property var track
    readonly property real buttonHeight: 18
    readonly property real buttonPadding: 1
    property bool isResizing: false

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: {
        track.height;
        if (track.hasLanes)
            track.lanesVisible;

        return track.getFullVisibleHeight();
    }
    opacity: Style.getOpacity(track.enabled, control.Window.active)

    Component {
        id: mainTrackView

        Item {
            id: mainTrackViewItem

            ColumnLayout {
                id: mainTrackViewColumnLayout

                spacing: 0
                anchors.fill: parent

                RowLayout {
                    id: topRow

                    readonly property real contentHeight: Math.max(trackNameLabel.implicitHeight, linkedButtons.implicitHeight)

                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: contentHeight
                    Layout.preferredHeight: contentHeight

                    IconImage {
                        id: trackIcon

                        readonly property real iconSize: 14

                        Layout.alignment: Qt.AlignTop | Qt.AlignBaseline
                        source: {
                            if (track.icon.startsWith("gnome-icon-library-"))
                                return Style.getIcon("gnome-icon-library", track.icon.substring(19) + ".svg");

                            if (track.icon.startsWith("fluentui-"))
                                return Style.getIcon("fluentui", track.icon.substring(9) + ".svg");

                            if (track.icon.startsWith("lorc-"))
                                return Style.getIcon("lorc", track.icon.substring(5) + ".svg");

                            if (track.icon.startsWith("jam-icons-"))
                                return Style.getIcon("jam-icons", track.icon.substring(10) + ".svg");

                            return Style.getIcon("zrythm-dark", track.icon + ".svg");
                        }
                        sourceSize.width: iconSize
                        sourceSize.height: iconSize
                        width: iconSize
                        height: iconSize
                        fillMode: Image.PreserveAspectFit
                        color: trackNameLabel.color
                    }

                    Label {
                        id: trackNameLabel

                        text: track.name
                        elide: Text.ElideRight
                        font: track.selected ? Style.buttonTextFont : Style.normalTextFont
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
                    }

                    LinkedButtons {
                        id: linkedButtons

                        layer.enabled: true
                        Layout.fillWidth: false
                        Layout.alignment: Qt.AlignRight | Qt.AlignBaseline | Qt.AlignTop

                        Button {
                            text: "M"
                            checkable: true
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding

                            ToolTip {
                                text: qsTr("Mute")
                            }

                        }

                        SoloButton {
                            id: soloButton

                            text: "S"
                            checkable: true
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                        }

                        RecordButton {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                            height: soloButton.height
                        }

                        layer.effect: DropShadowEffect {
                        }

                    }

                }

                RowLayout {
                    id: bottomRow

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignBottom

                    Label {
                        id: chordScalesLabel

                        text: qsTr("Scales")
                        font: Style.smallTextFont
                        Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                        visible: track.type === 3 // chord track
                    }

                    Item {
                        id: filler

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    LinkedButtons {
                        id: bottomRightButtons

                        visible: track.height > topRow.contentHeight + height + (mainTrackViewColumnLayout.Layout.topMargin + mainTrackViewColumnLayout.Layout.bottomMargin + control.spacing + 8)
                        layer.enabled: true
                        Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                        Layout.fillHeight: true

                        Button {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                            checkable: true
                            visible: track.hasLanes
                            checked: track.hasLanes && track.lanesVisible
                            icon.source: Style.getIcon("gnome-icon-library", "list-compact-symbolic.svg")
                            onClicked: {
                                track.lanesVisible = !track.lanesVisible;
                            }

                            ToolTip {
                                text: qsTr("Show lanes")
                            }

                        }

                        Button {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                            checkable: true
                            icon.source: Style.getIcon("zrythm-dark", "automation-4p.svg")

                            ToolTip {
                                text: qsTr("Show automation")
                            }

                        }

                        layer.effect: DropShadowEffect {
                        }

                    }

                }

            }

            Loader {
                property var resizeTarget: track

                sourceComponent: resizeHandle
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }

        }

    }

    Component {
        id: resizeHandle

        Rectangle {
            id: resizeHandleRect

            height: 6
            color: "transparent"

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            DragHandler {
                id: resizer

                property real startHeight: 0

                yAxis.enabled: true
                xAxis.enabled: false
                grabPermissions: PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlers
                onActiveChanged: {
                    control.isResizing = active;
                    if (active)
                        resizer.startHeight = resizeTarget.height;

                }
                onTranslationChanged: {
                    if (active) {
                        let newHeight = Math.max(resizer.startHeight + translation.y, 24);
                        resizeTarget.height = newHeight;
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                acceptedButtons: Qt.NoButton // Pass through mouse events to the DragHandler
            }

        }

    }

    Behavior on implicitHeight {
        // FIXME: stops working after some operations, not sure why
        enabled: !control.isResizing
        animation: Style.propertyAnimation
    }

    background: Rectangle {
        color: {
            let c = control.palette.window;
            if (control.hovered)
                c = Style.getColorBlendedTowardsContrast(c);

            if (track.selected || control.down)
                c = Style.getColorBlendedTowardsContrast(c);

            return c;
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: function(point) {
                track.setExclusivelySelected(true);
            }
        }

        Behavior on color {
            animation: Style.propertyAnimation
        }

    }

    contentItem: Item {
        id: trackContent

        anchors.fill: parent

        RowLayout {
            id: mainLayout

            anchors.fill: parent
            spacing: 4
            Layout.rightMargin: 6

            Rectangle {
                id: trackColor

                width: 6
                Layout.fillHeight: true
                color: track.color
            }

            ColumnLayout {
                id: allTrackComponentRows

                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Loader {
                    Layout.bottomMargin: 3
                    Layout.topMargin: 3
                    sourceComponent: mainTrackView
                    Layout.fillWidth: true
                    Layout.minimumHeight: track.height - Layout.bottomMargin - Layout.topMargin
                    Layout.maximumHeight: track.height - Layout.bottomMargin - Layout.topMargin
                }

                Loader {
                    id: lanesLoader

                    active: track.hasLanes && track.lanesVisible
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    sourceComponent: ColumnLayout {
                        id: lanesColumnLayout

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        ListView {
                            id: lanesListView

                            model: track.lanes
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            delegate: ItemDelegate {
                                readonly property var lane: modelData

                                height: lane.height
                                width: ListView.view.width

                                Label {
                                    text: lane.name
                                }

                                Loader {
                                    property var resizeTarget: lane

                                    sourceComponent: resizeHandle
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                }

                                Connections {
                                    function onHeightChanged() {
                                        // update the track height so things that depend on the track height also get updated
                                        track.heightChanged(track.height);
                                    }

                                    target: lane
                                }

                            }

                        }

                    }

                }

            }

            RowLayout {
                id: meters

                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    width: 4
                    color: "red"
                    Layout.fillHeight: true
                }

                Rectangle {
                    width: 4
                    color: "green"
                    Layout.fillHeight: true
                }

            }

        }

    }

}
