import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Button {
    id: root
    checkable: true

    palette {
        accent: Style.soloGreenColor
    }

    ToolTip {
        text: qsTr("Solo")
    }
}