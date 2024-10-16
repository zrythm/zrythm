import QtQuick
import QtQuick.Controls.Basic

TabBar {
    id: root

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    spacing: 4
    padding: 4

    background: Rectangle {
        color: root.palette.window
    }
}
