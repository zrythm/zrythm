import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property alias text: mainButton.text
    property alias menuItems: menuLoader.sourceComponent
    property color separatorColor: Qt.lighter(palette.button, 1.3)
    property bool directionUpward: false

    signal clicked()

    implicitWidth: Math.max(rowLayout.implicitWidth, 64)
    implicitHeight: 24

    RowLayout {
        id: rowLayout

        anchors.fill: parent
        spacing: 0

        ZrythmToolButton {
            id: mainButton

            // Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: implicitWidth
            text: "Undo Action"
            onClicked: root.clicked()
            clip: true
            anchors.rightMargin: -width

            background: Rectangle {
                anchors.fill: mainButton
                anchors.rightMargin: -mainButton.width
                radius: mainButton.backgroundRadius
                visible: false
            }

        }

        Rectangle {
            width: 1
            Layout.fillHeight: true
            color: root.separatorColor
        }

        ZrythmToolButton {
            id: arrowButton

            Layout.fillHeight: true
            Layout.preferredWidth: root.height
            text: root.directionUpward ? "▲" : "▼"
            onClicked: menuLoader.item.open()
            clip: true
            anchors.leftMargin: -width
            Component.onCompleted: {
                contentItem.font.pixelSize = root.height * 0.38;
            }

            background: Rectangle {
                anchors.fill: arrowButton
                anchors.leftMargin: -arrowButton.width
                radius: arrowButton.backgroundRadius
                visible: false
            }

        }

    }

    Loader {
        id: menuLoader

        y: root.directionUpward ? -item.height : root.height

        sourceComponent: ZrythmMenu {
            Action {
                text: "Fill Me"
            }

        }

    }

}
