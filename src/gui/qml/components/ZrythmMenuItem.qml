import QtQuick
import QtQuick.Controls.Basic

MenuItem {
    id: menuItem

    implicitHeight: 24

    contentItem: Text {
        text: menuItem.text
        color: menuItem.enabled ? (menuItem.highlighted ? palette.highlightedText : palette.windowText) : palette.disabled
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        leftPadding: 8
    }

    background: Rectangle {
        implicitWidth: 200
        color: menuItem.highlighted ? palette.highlight : "transparent"
    }

    arrow: Canvas {
        x: parent.width - width
        implicitWidth: 40
        implicitHeight: 40
        visible: menuItem.subMenu
        onPaint: {
            var ctx = getContext("2d");
            ctx.fillStyle = menuItem.highlighted ? "#ffffff" : "#21be2b";
            ctx.moveTo(15, 15);
            ctx.lineTo(width - 15, height / 2);
            ctx.lineTo(15, height - 15);
            ctx.closePath();
            ctx.fill();
        }
    }

    indicator: Item {
        implicitWidth: 40
        implicitHeight: 40

        Rectangle {
            width: 26
            height: 26
            anchors.centerIn: parent
            visible: menuItem.checkable
            border.color: "#21be2b"
            radius: 3

            Rectangle {
                width: 14
                height: 14
                anchors.centerIn: parent
                visible: menuItem.checked
                color: "#21be2b"
                radius: 2
            }

        }

    }

}
