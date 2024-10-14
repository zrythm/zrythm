import QtQuick
import QtQuick.Controls.Basic

Rectangle {

    required property Menu menu: Menu {}

    implicitWidth: menu.contentItem.childrenRect.width
    implicitHeight: menu.contentHeight
    color: palette.window
    border.color: palette.mid
    border.width: 1
}
