import QtQuick
import QtQuick.Controls.Basic

ToolSeparator {
    id: zrythmToolSeparator

    contentItem: Rectangle {
        implicitWidth: 1
        implicitHeight: 16
        color: palette.mid
    }

    padding: 6
    topPadding: 2
    bottomPadding: 2

    verticalPadding: orientation === Qt.Horizontal ? padding : 0
    horizontalPadding: orientation === Qt.Vertical ? padding : 0
}