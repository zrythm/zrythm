// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle 1.0

ListView {
    id: automationTracksListView

    property var track

    interactive: false
    clip: true
    implicitHeight: contentHeight

    Connections {
        function onRowsInserted() {
            track.fullVisibleHeightChanged();
        }

        function onRowsRemoved() {
            track.fullVisibleHeightChanged();
        }

        function onDataChanged() {
            track.fullVisibleHeightChanged();
        }

        function onModelReset() {
            track.fullVisibleHeightChanged();
        }

        target: automationTracksListView.model
    }

    Behavior on implicitHeight {
        animation: Style.propertyAnimation
    }

    model: AutomationTracklistProxyModel {
        sourceModel: track.automationTracks
        showOnlyVisible: true
        showOnlyCreated: true
    }

    delegate: ItemDelegate {
        readonly property var automationTrack: modelData

        height: automationTrack.height
        width: ListView.view.width

        ButtonGroup {
            id: automationModeGroup

            exclusive: true
        }

        ColumnLayout {
            id: automationColumnLayout

            readonly property font buttonFont: Style.semiBoldTextFont

            spacing: 4

            anchors {
                fill: parent
                topMargin: control.contentTopMargins
                bottomMargin: 6
            }

            RowLayout {
                id: automationTopRowLayout

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                spacing: 6

                Button {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
                    text: automationTrack.label
                    icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
                    styleHeight: control.buttonHeight
                    padding: control.buttonPadding
                    font: automationColumnLayout.buttonFont
                    Component.onCompleted: {
                        if (background && background instanceof Rectangle)
                            background.radius = Style.textFieldRadius;

                    }

                    ToolTip {
                        text: qsTr("Change automatable")
                    }

                }

                Label {
                    Layout.fillWidth: false
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
                    text: automationTrack.height // "1.0"
                    font: Style.smallTextFont
                }

            }

            RowLayout {
                id: automationBottomRowLayout

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignBottom
                visible: automationTrack.height > automationTopRowLayout.height + height + 6

                LinkedButtons {
                    id: automationModeButtons

                    Layout.fillWidth: false
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
                    layer.enabled: true

                    Button {
                        text: qsTr("On")
                        styleHeight: control.buttonHeight
                        padding: control.buttonPadding
                        ButtonGroup.group: automationModeGroup
                        checkable: true
                        checked: automationTrack.automationMode === 0
                        font: automationColumnLayout.buttonFont
                        onClicked: {
                            automationTrack.automationMode = 0;
                        }
                    }

                    Button {
                        text: automationTrack.recordMode === 0 ? qsTr("Touch") : qsTr("Latch")
                        styleHeight: control.buttonHeight
                        padding: control.buttonPadding
                        ButtonGroup.group: automationModeGroup
                        checkable: true
                        checked: automationTrack.automationMode === 1
                        font: automationColumnLayout.buttonFont
                        onClicked: {
                            if (automationTrack.automationMode === 1)
                                automationTrack.recordMode = automationTrack.recordMode === 0 ? 1 : 0;

                            automationTrack.automationMode = 1;
                        }
                    }

                    Button {
                        text: qsTr("Off")
                        styleHeight: control.buttonHeight
                        padding: control.buttonPadding
                        ButtonGroup.group: automationModeGroup
                        checkable: true
                        checked: automationTrack.automationMode === 2
                        font: automationColumnLayout.buttonFont
                        onClicked: {
                            automationTrack.automationMode = 2;
                        }
                    }

                }

                Item {
                    id: bottomAutomationSpacer

                    Layout.fillWidth: true
                    Layout.fillHeight: false
                }

                LinkedButtons {
                    Layout.fillWidth: false
                    Layout.fillHeight: false
                    Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
                    layer.enabled: true

                    Button {
                        id: removeAutomationTrackButton

                        // icon.source: ResourceManager.getIconUrl("zrythm-dark", "remove.svg")
                        text: "-"
                        styleHeight: control.buttonHeight
                        padding: control.buttonPadding
                        onClicked: {
                            track.automationTracks.hideAutomationTrack(automationTrack);
                        }

                        ToolTip {
                            text: qsTr("Hide automation lane")
                        }

                        font {
                            family: Style.fontFamily
                            pixelSize: 14
                            bold: true
                        }

                    }

                    Button {
                        id: addAutomationTrackButton

                        // icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")
                        text: "+"
                        styleHeight: control.buttonHeight
                        padding: control.buttonPadding
                        onClicked: {
                            track.automationTracks.showNextAvailableAutomationTrack(automationTrack);
                        }

                        ToolTip {
                            text: qsTr("Add automation lane")
                        }

                        font {
                            family: Style.fontFamily
                            pixelSize: 14
                            bold: true
                        }

                    }

                }

            }

        }

        Loader {
            property var resizeTarget: automationTrack

            sourceComponent: resizeHandle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        Connections {
            function onHeightChanged() {
                track.fullVisibleHeightChanged();
            }

            target: automationTrack
        }

    }

}
