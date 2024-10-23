import QtQuick 
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ZrythmArranger {
    id: root

    required property bool pinned
    required property var timeline
    editorSettings: timeline
}