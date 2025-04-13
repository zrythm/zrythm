// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: control

    required property var track // connected automatically when used as a delegate for a Tracklist model
    readonly property real buttonHeight: 18
    readonly property real buttonPadding: 1
    readonly property real contentTopMargins: 1
    readonly property real contentBottomMargins: 3
    property bool isResizing: false

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: track.fullVisibleHeight
    opacity: Style.getOpacity(track.enabled, control.Window.active)

    Connections {
        function onHeightChanged() {
            track.fullVisibleHeightChanged();
        }

        function onLanesVisibleChanged() {
            track.fullVisibleHeightChanged();
        }

        function onAutomationVisibleChanged() {
            track.fullVisibleHeightChanged();
        }

        ignoreUnknownSignals: true
        target: track
    }

    Component {
        id: mainTrackView

        Item {
            id: mainTrackViewItem

            ColumnLayout {
                id: mainTrackViewColumnLayout

                spacing: 0

                anchors {
                    fill: parent
                    topMargin: control.contentTopMargins
                    bottomMargin: control.contentBottomMargins
                }

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
                                return ResourceManager.getIconUrl("gnome-icon-library", track.icon.substring(19) + ".svg");

                            if (track.icon.startsWith("fluentui-"))
                                return ResourceManager.getIconUrl("fluentui", track.icon.substring(9) + ".svg");

                            if (track.icon.startsWith("lorc-"))
                                return ResourceManager.getIconUrl("lorc", track.icon.substring(5) + ".svg");

                            if (track.icon.startsWith("jam-icons-"))
                                return ResourceManager.getIconUrl("jam-icons", track.icon.substring(10) + ".svg");

                            return ResourceManager.getIconUrl("zrythm-dark", track.icon + ".svg");
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

                        MuteButton {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                        }

                        SoloButton {
                            id: soloButton

                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                        }

                        RecordButton {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                            height: soloButton.height
                            visible: track.isRecordable
                            checked: track.isRecordable && track.recording
                            onClicked: {
                              track.recording = !track.recording;
                            }
                        }

                        layer.effect: DropShadowEffect {
                        }

                    }

                }

                RowLayout {
                    id: bottomRow

                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignBottom
                    visible: track.height > control.buttonHeight + height + 12

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

                        layer.enabled: true
                        Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                        Layout.fillHeight: true

                        Button {
                            styleHeight: control.buttonHeight
                            padding: control.buttonPadding
                            checkable: true
                            visible: track.hasLanes
                            checked: track.hasLanes && track.lanesVisible
                            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "list-compact-symbolic.svg")
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
                            visible: track.isAutomatable
                            checked: track.isAutomatable && track.automationVisible
                            icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
                            onClicked: {
                                track.automationVisible = !track.automationVisible;
                            }

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

            Rectangle {
                anchors.centerIn: parent
                height: 2
                width: 24
                color: Qt.alpha(Style.backgroundAppendColor, 0.2)
                radius: 2
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
                tracklist.setExclusivelySelectedTrack(control.track);
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
                    visible: active

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
                            implicitHeight: contentHeight

                            delegate: ItemDelegate {
                                readonly property var lane: modelData

                                height: lane.height
                                width: ListView.view.width

                                RowLayout {
                                    spacing: 2

                                    anchors {
                                        fill: parent
                                        topMargin: control.contentTopMargins
                                        bottomMargin: control.contentBottomMargins
                                    }

                                    Label {
                                        text: lane.name
                                        Layout.fillWidth: true
                                        Layout.fillHeight: false
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                    }

                                    LinkedButtons {
                                        Layout.fillWidth: false
                                        Layout.fillHeight: false
                                        Layout.alignment: Qt.AlignRight | Qt.AlignTop

                                        SoloButton {
                                            id: soloButton

                                            styleHeight: control.buttonHeight
                                            padding: control.buttonPadding
                                        }

                                        MuteButton {
                                            styleHeight: control.buttonHeight
                                            padding: control.buttonPadding
                                        }

                                    }

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
                                        track.fullVisibleHeightChanged();
                                    }

                                    target: lane
                                }

                            }

                        }

                    }

                }

                Loader {
                    id: automationLoader

                    active: track.isAutomatable && track.automationVisible
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    sourceComponent: AutomationTracksListView {
                        id: automationTracksListView

                        track: control.track
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                }

            }

            Loader {
                id: audioMetersLoader

                active: track.hasChannel && track.channel.leftAudioOut
                visible: active
                Layout.fillHeight: true
                Layout.fillWidth: false

                sourceComponent: RowLayout {
                    id: meters

                    spacing: 0
                    anchors.fill: parent

                    Meter {
                        Layout.fillHeight: true
                        Layout.preferredWidth: width
                        port: track.channel.leftAudioOut
                    }

                    Meter {
                        Layout.fillHeight: true
                        Layout.preferredWidth: width
                        port: track.channel.rightAudioOut
                    }

                }

            }

            Loader {
                id: midiMetersLoader

                active: track.hasChannel && track.channel.midiOut
                visible: active
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.preferredWidth: 4

                sourceComponent: Meter {
                    anchors.fill: parent
                    port: track.channel.midiOut
                }

            }

        }

    }

}
