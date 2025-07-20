// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
        sourceModel: track.automatableTrackMixin.automationTracklist
        showOnlyVisible: true
        showOnlyCreated: true
    }

    delegate: ItemDelegate {
        required property var automationTrackHolder
        readonly property var automationTrack: automationTrackHolder.automationTrack

        height: automationTrackHolder.height
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
                    text: automationTrackHolder.automationTrack.parameter.label
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
                    text: {
                        automationTrackHolder.automationTrack.parameter.baseValue;
                        return automationTrackHolder.automationTrack.parameter.range.convertFrom0To1(automationTrackHolder.automationTrack.parameter.currentValue());
                    }
                    font: Style.smallTextFont
                }
            }

            RowLayout {
                id: automationBottomRowLayout

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignBottom
                visible: automationTrackHolder.height > automationTopRowLayout.height + height + 6

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
                            track.automatableTrackMixin.automationTracklist.hideAutomationTrack(automationTrack);
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
                            track.automatableTrackMixin.automationTracklist.showNextAvailableAutomationTrack(automationTrack);
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
            property var resizeTarget: automationTrackHolder

            sourceComponent: resizeHandle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        Connections {
            function onHeightChanged() {
                track.fullVisibleHeightChanged();
            }

            target: automationTrackHolder
        }
    }
}
