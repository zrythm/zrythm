import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic 6.7
import QtQuick.Layouts

ColumnLayout {
    id: root

    property alias icon: placeholderIcon
    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias action: actionButton.action
    property alias buttonText: actionButton.text

    anchors.centerIn: parent
    spacing: 10
    width: Math.min(parent.width - 40, parent.width * 0.8)

    Image {
        id: placeholderIcon

        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 128
        Layout.preferredHeight: 128
        Layout.bottomMargin: 16
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        // these ensure SVGs are scaled and not stretched
        sourceSize.width: width
        sourceSize.height: height
    }

    Label {
        id: titleLabel

        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pointSize: 20
        font.bold: true
    }

    Label {
        id: descriptionLabel

        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pointSize: 10.5
        text: ""
        visible: text !== ""
        onLinkActivated: (link) => {
            console.log("Link clicked:", link);
            Qt.openUrlExternally(link);
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }

    }

    PlainButton {
        id: actionButton

        Layout.topMargin: 10
        Layout.alignment: Qt.AlignHCenter
        visible: action || text
    }

}
