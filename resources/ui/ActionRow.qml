import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    default property alias content: suffixLayout.data

    implicitWidth: Math.min(parent.width - 40, parent.width * 0.8)
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout

        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                id: titleLabel

                text: root.title
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                id: subtitleLabel

                text: root.subtitle
                visible: text !== ""
                opacity: 0.7
                font.pixelSize: titleLabel.font.pixelSize * 0.9
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

        }

        RowLayout {
            id: suffixLayout

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            spacing: 8
        }

    }

/*
    Rectangle {
        anchors.left: anchors.left
        anchors.right: anchors.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#20000000" // Semi-transparent black
    }
    */

}
