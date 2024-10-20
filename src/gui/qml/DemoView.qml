// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
    id: root

    readonly property real spacing: 10

    visible: true
    title: "Demo View"
    width: 1280
    height: 720

    Flow {
        id: outerFlow

        anchors.fill: parent

        Repeater {
            model: [true, false]

            Flow {
                id: enabledRepeater

                property bool enabledBoolData: modelData

                spacing: root.spacing
                width: parent.width

                Flow {
                    id: allWidgetsContainer

                    padding: 12
                    spacing: root.spacing
                    width: parent.width

                    Repeater {
                        id: buttonRepeater

                        model: 3

                        Flow {
                            spacing: root.spacing

                            Button {
                                text: "Button"
                                checkable: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData

                                ToolTip {
                                    text: "Some long tooltip text"
                                }

                            }

                            Button {
                                text: "Button"
                                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                                checkable: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData
                            }

                            Button {
                                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                                checkable: modelData === 1
                                checked: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData
                            }

                            RoundButton {
                                text: "Button"
                                checkable: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData

                                ToolTip {
                                    text: "Some long tooltip text"
                                }

                            }

                            RoundButton {
                                text: "Button"
                                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                                checkable: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData
                            }

                            RoundButton {
                                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                                checkable: modelData === 1
                                checked: modelData === 1
                                highlighted: modelData === 2
                                enabled: enabledRepeater.enabledBoolData
                            }

                        }

                    }

                    ComboBox {
                        model: ['日本語', 'bbbb long text', 'short']
                        enabled: enabledRepeater.enabledBoolData
                    }

                }

            }

        }

    }

    header: ToolBar {
        id: toolBar

        RowLayout {
            ToolButton {
                text: "Tool Button"
            }

            ToolButton {
                text: "Tool Button"
                icon.source: Style.getIcon("zrythm-dark", "zrythm-monochrome.svg")
            }

            ToolButton {
                icon.source: Style.getIcon("zrythm-dark", "zrythm-monochrome.svg")
            }

            ToolSeparator {}

            ToolButton {
                text: "Tool Button"
                icon.source: Style.getIcon("zrythm-dark", "zrythm-monochrome.svg")
            }

        }

    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")

            Action {
                text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
            }

            Action {
                text: qsTr("Open")
            }

        }

        Menu {
            title: qsTr("&Edit")

            Action {
                text: qsTr("Undo")
            }

        }

        Menu {
            title: qsTr("&Help")

            MenuItem {
                text: qsTr("Something")
                onTriggered: {
                }
            }

            Action {
                text: "Something 2"
            }

            Action {
                text: "Something 3"
            }

            Action {
                text: "Something 4"
            }

            Action {
                text: "Somet&hing 5"
            }

            Action {
                text: "Something 6"
            }

        }

    }

}
