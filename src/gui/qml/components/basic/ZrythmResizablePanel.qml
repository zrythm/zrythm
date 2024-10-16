import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property string title: "Panel"
    property bool collapsed: false
    property alias content: contentLoader.sourceComponent
    property int collapsedSize: 30
    property bool vertical: true

    implicitWidth: vertical ? parent.width : (collapsed ? collapsedSize : contentLoader.implicitWidth + collapsedSize)
    implicitHeight: vertical ? (collapsed ? collapsedSize : contentLoader.implicitHeight + collapsedSize) : parent.height

    padding: 0
    background: Rectangle {
        color: "#2C2C2C"
        border.color: "#1C1C1C"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header
            Layout.fillHeight: true
            Layout.preferredWidth: root.vertical ? parent.width : root.collapsedSize
            Layout.preferredHeight: root.vertical ? root.collapsedSize : parent.height
            color: "#3C3C3C"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 5

                Label {
                    text: root.title
                    color: "white"
                    Layout.fillWidth: true
                    rotation: root.vertical ? 0 : -90
                    Layout.alignment: root.vertical ? Qt.AlignVCenter : Qt.AlignBottom
                }

                Button {
                    text: root.collapsed ? "+" : "-"
                    onClicked: root.collapsed = !root.collapsed
                    implicitWidth: 20
                    implicitHeight: 20
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                }
            }
        }

        Loader {
            id: contentLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.collapsed
        }
    }
}
