// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

ArrangerObjectBaseView {
  id: root

  property TimeSignatureObject timeSignatureObject: root.arrangerObject as TimeSignatureObject

  width: textMetrics.width + 2 * Style.buttonPadding

  ContextMenu.menu: Menu {
    Menu {
      title: qsTr("Set Beats Per Bar")

      Repeater {
        model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]

        MenuItem {
          readonly property int beatsPerBar: modelData
          required property var modelData

          text: beatsPerBar

          onTriggered: propertyOperator.setValueAffectingTempoMap("numerator", beatsPerBar)
        }
      }
    }

    Menu {
      title: qsTr("Set Beat Unit")

      Repeater {
        model: [2, 4, 8, 16]

        MenuItem {
          readonly property int beatUnit: modelData
          required property var modelData

          text: beatUnit

          onTriggered: propertyOperator.setValueAffectingTempoMap("denominator", beatUnit)
        }
      }
    }
  }

  QObjectPropertyOperator {
    id: propertyOperator

    Component.onCompleted: {
      if (root.arrangerObject && root.undoStack) {
        propertyOperator.currentObject = root.arrangerObject;
        propertyOperator.undoStack = root.undoStack;
      }
    }
  }

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: Style.toolButtonRadius
  }

  Text {
    id: nameText

    color: root.palette.text
    font: root.font
    padding: Style.buttonPadding
    text: "%1/%2".arg(root.timeSignatureObject.numerator).arg(root.timeSignatureObject.denominator)
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }
}
