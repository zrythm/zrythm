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
                // put things in here

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

                    ProgressBar {
                        indeterminate: true
                        enabled: enabledRepeater.enabledBoolData
                    }

                    ProgressBar {
                        from: 0
                        to: 100
                        value: 50
                        enabled: enabledRepeater.enabledBoolData

                        // Dummy animation to move the progress bar back and forth
                        SequentialAnimation on value {
                            loops: Animation.Infinite

                            NumberAnimation {
                                from: 0
                                to: 100
                                duration: 2000
                                easing.type: Style.animationEasingType
                            }

                            NumberAnimation {
                                from: 100
                                to: 0
                                duration: 2000
                                easing.type: Style.animationEasingType
                            }

                        }

                    }

                    TabBar {
                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "piano-roll.svg")
                            text: qsTr("Editor")
                            enabled: enabledRepeater.enabledBoolData
                        }

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "mixer.svg")
                            text: qsTr("Mixer")
                            enabled: enabledRepeater.enabledBoolData
                        }

                        TabButton {
                            icon.source: Style.getIcon("gnome-icon-library", "encoder-knob-symbolic.svg")
                            text: qsTr("Modulators")
                            enabled: enabledRepeater.enabledBoolData
                        }

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "chord-pad.svg")
                            text: "Chord Pad"
                            enabled: enabledRepeater.enabledBoolData
                        }

                    }

                    TabBar {
                        enabled: enabledRepeater.enabledBoolData

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "track-inspector.svg")
                            text: qsTr("Track Inspector")
                        }

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "plug.svg")
                            text: qsTr("Plugin Inspector")
                        }

                    }

                    TabBar {
                        enabled: enabledRepeater.enabledBoolData

                        TabButton {
                            icon.source: Style.getIcon("gnome-icon-library", "shapes-large-symbolic.svg")

                            ToolTip {
                                text: qsTr("Plugin Browser")
                            }

                        }

                        TabButton {
                            icon.source: Style.getIcon("gnome-icon-library", "file-cabinet-symbolic.svg")

                            ToolTip {
                                text: qsTr("File Browser")
                            }

                        }

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "speaker.svg")

                            ToolTip {
                                text: qsTr("Monitor Section")
                            }

                        }

                        TabButton {
                            icon.source: Style.getIcon("zrythm-dark", "minuet-chords.svg")

                            ToolTip {
                                text: qsTr("Chord Preset Browser")
                            }

                        }

                    }

                    TextField {
                        placeholderText: "Placeholder text"
                        enabled: enabledRepeater.enabledBoolData
                    }

                    TextField {
                        placeholderText: "Placeholder text"
                        text: "Text"
                        enabled: enabledRepeater.enabledBoolData
                    }

                }

            }

        }

    }

    header: ToolBar {
        id: toolBar

        RowLayout {
            Repeater {
                // put widgets here

                model: [true, false]

                RowLayout {
                    id: enabledRepeater

                    property bool enabledBoolData: modelData

                    Repeater {
                        model: [true, false]

                        Flow {
                            id: checkedFlow

                            property bool checkableBoolData: modelData

                            ToolButton {
                                text: "Tool Button"
                                enabled: enabledRepeater.enabledBoolData
                                checkable: checkedFlow.checkableBoolData
                            }

                            ToolButton {
                                text: "Tool Button"
                                icon.source: Style.getIcon("zrythm-dark", "zrythm-monochrome.svg")
                                enabled: enabledRepeater.enabledBoolData
                                checkable: checkedFlow.checkableBoolData
                                checked: checkedFlow.checkableBoolData
                            }

                            ToolButton {
                                icon.source: Style.getIcon("zrythm-dark", "zrythm-monochrome.svg")
                                enabled: enabledRepeater.enabledBoolData
                                checkable: checkedFlow.checkableBoolData
                            }

                        }

                    }

                    ToolSeparator {
                        enabled: enabledRepeater.enabledBoolData
                    }

                    ZrythmSplitButton {
                        id: undoBtn

                        tooltipText: qsTr("Undo")
                        menuTooltipText: qsTr("Undo Multiple")
                        iconSource: Style.getIcon("zrythm-dark", "edit-undo.svg")
                        enabled: enabledRepeater.enabledBoolData

                        menuItems: Menu {
                            Action {
                                text: qsTr("Undo Move")
                            }

                            Action {
                                text: qsTr("Undo Cut")
                            }

                        }

                    }

                    ZrythmSplitButton {
                        id: redoBtn

                        tooltipText: qsTr("Redo")
                        menuTooltipText: qsTr("Redo Multiple")
                        iconSource: Style.getIcon("zrythm-dark", "edit-redo.svg")
                        enabled: enabledRepeater.enabledBoolData
                    }

                }

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
            title: qsTr("&View")

            Menu {
                title: qsTr("Appearance")

                Action {
                    text: qsTr("Switch Light/Dark Theme")
                    onTriggered: {
                        Style.darkMode = !Style.darkMode;
                    }
                }

                Menu {
                    title: qsTr("Theme Color")

                    Action {
                        text: qsTr("Zrythm Orange")
                        onTriggered: {
                            Style.primaryColor = Style.zrythmColor;
                        }
                    }
                    Action {
                        text: qsTr("Celestial Blue")
                        onTriggered: {
                            Style.primaryColor = Style.celestialBlueColor;
                        }
                    }
                        Action {
                        text: qsTr("Jonquil Yellow")
                        onTriggered: {
                            Style.primaryColor = Style.jonquilYellowColor;
                        }
                    }
                }

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
