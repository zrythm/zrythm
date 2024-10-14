import QtQuick
import QtQuick.Controls.Basic

Menu {
    id: root

    delegate: ZrythmMenuItem {
    }

    background: ZrythmMenuBackground {
        menu: root
    }

}
