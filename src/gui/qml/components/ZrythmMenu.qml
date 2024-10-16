import QtQuick
import QtQuick.Controls.Basic

Menu {
    id: root

    popupType: Popup.Native // auto-fallbacks to Window, then normal

    delegate: ZrythmMenuItem {
    }

    background: ZrythmMenuBackground {
        id: menuBackground
        implicitWidth: root.contentItem.childrenRect.width
        implicitHeight: root.contentHeight
    }

}
