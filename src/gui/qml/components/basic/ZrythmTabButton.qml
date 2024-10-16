import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

TabButton {
    id: root

    property real backgroundRadius: 6
    property string iconSource: "" // Qt.resolvedUrl("../icons/zrythm-dark/edit-undo.svg")
    property color buttonTextColor: palette.buttonText
    property color buttonCheckedColor: palette.accent

    padding: 4
    implicitHeight: 26
    text: ""
    states: [
        State {
            name: "pressed"
            when: root.down

            PropertyChanges {
                target: root
                buttonTextColor: palette.highlightedText
            }

            PropertyChanges {
                target: root.background
                color: root.buttonCheckedColor
            }

        },
        State {
            name: "checked"
            when: root.enabled && (root.checked)

            PropertyChanges {
                target: root.background
                visible: true
                color: root.buttonCheckedColor
                opacity: 0.8
            }

        },
        State {
            name: "hovered"
            when: root.enabled && root.hovered

            PropertyChanges {
                target: root.background
                visible: true
                color: Qt.darker(palette.button, 1.1)
            }

        }
    ]

    font {
        pointSize: 10
        weight: Font.Medium
    }

    Component {
        id: buttonTextComponent

        Text {
            text: root.text
            opacity: root.enabled ? 1 : 0.3
            color: root.buttonTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: root.font
        }

    }

    Component {
        id: buttonIconComponent

        Image {
            id: buttonImage

            readonly property int iconSize: root.availableHeight

            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            sourceSize.width: iconSize
            width: iconSize
            opacity: root.enabled ? 1 : 0.3
        }

    }

    contentItem: Loader {
        sourceComponent: root.text === "" ? iconOnlyComponent : (root.iconSource === "" ? textOnlyComponent : iconTextComponent)

        Component {
            id: iconTextComponent

            RowLayout {
                spacing: 4
                anchors.centerIn: parent
                anchors.fill: parent

                Item {
                    id: leftSpacer

                    Layout.fillWidth: true
                }

                Loader {
                    sourceComponent: buttonIconComponent
                    Layout.alignment: Qt.AlignVCenter
                    // Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Loader {
                    sourceComponent: buttonTextComponent
                    Layout.alignment: Qt.AlignVCenter
                    // Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Item {
                    id: rightSpacer

                    Layout.fillWidth: true
                }

            }

        }

        Component {
            id: textOnlyComponent

            Loader {
                sourceComponent: buttonTextComponent
                anchors.centerIn: parent
                anchors.fill: parent
            }

        }

        Component {
            id: iconOnlyComponent

            Loader {
                sourceComponent: buttonIconComponent
                anchors.centerIn: parent
                anchors.fill: parent
            }

        }

    }

    background: Rectangle {
        id: backgroundRect

        color: palette.button
        opacity: root.enabled ? 1 : 0.3
        visible: true
        radius: root.backgroundRadius
    }

}
