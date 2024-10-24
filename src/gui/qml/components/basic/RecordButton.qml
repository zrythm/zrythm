import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Button {
    id: root
    checkable: true

    icon.source: Style.getIcon("zrythm-dark", "record.svg")

    palette {
        buttonText: Style.dangerColor
        accent: Style.dangerColor
    }
}