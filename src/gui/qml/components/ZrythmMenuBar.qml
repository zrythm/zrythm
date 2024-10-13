import QtQuick
import QtQuick.Controls.Basic

MenuBar {
    id: menuBar

    readonly property real menuHeight: 18
    readonly property real menuItemWidth: 24

    Menu { title: qsTr("&File") }
    Menu { title: qsTr("&Edit") }
    Menu { title: qsTr("&View") }
    Menu { title: qsTr("&Help") }

    delegate: MenuBarItem {
        id: menuBarItem

        contentItem: Text {
            text: menuBarItem.text
            font: menuBarItem.font
            opacity: enabled ? 1.0 : 0.3
            color: menuBarItem.highlighted ? palette.highlightedText : palette.buttonText
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: menuItemWidth
            implicitHeight: menuHeight
            opacity: enabled ? 1 : 0.3
            color: menuBarItem.highlighted ? palette.highlight : "transparent"
        }
    }

    background: Rectangle {
        implicitWidth: menuItemWidth
        implicitHeight: menuHeight
        color: palette.window

        /*
        Rectangle {
            color: palette.accent
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
        }
        */
    }
}
