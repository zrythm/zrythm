import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Zrythm 1.0

ColumnLayout {
    spacing: 0

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: centerTabBar.currentIndex

        ZrythmArranger {
            id: timelineArranger

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            id: portConnectionsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            id: midiCcBindingsTab

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

    }

    ZrythmTabBar {
        id: centerTabBar

        Layout.fillWidth: true

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/roadmap.svg")
            text: qsTr("Timeline")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/connector.svg")
            text: qsTr("Port Connections")
        }

        ZrythmTabButton {
            iconSource: Qt.resolvedUrl("../icons/zrythm-dark/signal-midi.svg")
            text: qsTr("Midi CC Bindings")
        }

    }

}
