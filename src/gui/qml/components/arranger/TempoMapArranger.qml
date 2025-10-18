// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

Arranger {
  id: root

  required property int laneHeight
  required property int laneSpacing

  function beginObjectCreation(x: real, y: real): var {
  }

  enableYScroll: false
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

  content: ListView {
    id: tracksListView

    anchors.fill: parent
    interactive: false
    model: listTypeModel
    spacing: root.laneSpacing

    delegate: Item {
      id: trackDelegate

      readonly property bool isTempo: type === "Tempo"
      readonly property bool isTimeSignature: !isTempo
      required property string type

      Item {
        id: mainTrackObjectsContainer

        Layout.fillWidth: true
        Layout.maximumHeight: root.laneHeight
        Layout.minimumHeight: root.laneHeight

        Repeater {
          id: scalesRepeater

          model: trackDelegate.isTempo ? root.tempoMap.tempoEvents : root.tempoMap.timeSignatureEvents

          delegate: Loader {
            id: scaleLoader

            required property var model
            readonly property real tempoEventX: model.tick * root.ruler.pxPerTick

            active: tempoEventX + 100 + Style.scrollLoaderBufferPx >= root.scrollX && tempoEventX <= (root.scrollX + root.scrollViewWidth + Style.scrollLoaderBufferPx)
            asynchronous: true
            visible: status === Loader.Ready

            sourceComponent: Component {
              Label {
                id: scaleItem

                property var tempoEvent: scaleLoader.model

                font.bold: true
                text: tempoEvent.bpm + " (" + (tempoEvent.curve === 0 ? "constant" : "linear") + ")"
                x: scaleLoader.tempoEventX
              }
            }
          }
        }
      }
    }
  }

  ListModel {
    id: listTypeModel

    ListElement {
      type: "Tempo"
    }

    ListElement {
      type: "Time Signature"
    }
  }
}
