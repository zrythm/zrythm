import QtQuick
import QtQuick.Controls.Basic

Menu {
    id: root

    // popupType: Qt.platform.os === "osx" ? Popup.Native : Popup.Window

    delegate: ZrythmMenuItem {
    }

    background: ZrythmMenuBackground {
        id: menuBackground
        implicitWidth: root.contentItem.childrenRect.width
        implicitHeight: root.contentHeight
    }

    Component.onCompleted: {
        if (Qt.majorVersion > 6 || (Qt.majorVersion === 6 && Qt.minorVersion >= 8)) {
            if (Qt.platform.os === "osx") {
                root.popupType = Popup.Native;
            } else {
                root.popupType = Popup.Window;
            } 
        }
    }
}
