import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Button {
    id: root

    checkable: true
    text: "M"

    ToolTip {
        text: qsTr("Mute")
    }

}
