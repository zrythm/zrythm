// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import Zrythm

ArrangerObjectBaseView {
  id: root

  property TempoObject tempoObject: root.arrangerObject as TempoObject

  width: textMetrics.width + 2 * Style.buttonPadding

  ContextMenu.menu: Menu {
    Menu {
      title: qsTr("Set Tempo")

      Repeater {
        model: [90, 100, 110, 120, 130, 140, 150, 160]

        MenuItem {
          readonly property int bpm: modelData
          required property var modelData

          text: bpm

          onTriggered: propertyOperator.setValueAffectingTempoMap("tempo", bpm)
        }
      }
    }

    Menu {
      title: qsTr("Curve")

      MenuItem {
        text: qsTr("Constant")

        onTriggered: propertyOperator.setValueAffectingTempoMap("curve", TempoEventWrapper.Constant)
      }

      MenuItem {
        text: qsTr("Linear")

        onTriggered: propertyOperator.setValueAffectingTempoMap("curve", TempoEventWrapper.Linear)
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
    text: "%1 (%2)".arg(root.tempoObject.tempo).arg(root.tempoObject.curve === TempoEventWrapper.Constant ? "constant" : "linear")
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }

  onObjectDoubleClicked: tempoEditDialog.open()

  Dialog {
    id: tempoEditDialog

    modal: true
    popupType: Popup.Window
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: qsTr("Edit Tempo")

    contentItem: ColumnLayout {
      spacing: Style.buttonPadding

      Label {
        text: qsTr("BPM:")
      }

      DoubleSpinBox {
        id: tempoSpinBox

        Layout.fillWidth: true
        decimals: 1
        editable: true
        from: 1.0
        stepSize: 1.0
        to: 999.0
        value: root.tempoObject.tempo
      }

      Label {
        text: qsTr("Curve:")
      }

      ComboBox {
        id: curveComboBox

        Layout.fillWidth: true
        model: [
          {
            "text": qsTr("Constant"),
            "value": TempoEventWrapper.Constant
          },
          {
            "text": qsTr("Linear"),
            "value": TempoEventWrapper.Linear
          }
        ]
        textRole: "text"
        Component.onCompleted: curveComboBox.currentIndex = root.tempoObject.curve === TempoEventWrapper.Linear ? 1 : 0
      }
    }

    onAboutToShow: {
      tempoSpinBox.value = root.tempoObject.tempo;
      curveComboBox.currentIndex = root.tempoObject.curve === TempoEventWrapper.Linear ? 1 : 0;
    }

    onAccepted: {
      propertyOperator.setValueAffectingTempoMap("tempo", tempoSpinBox.value);
      propertyOperator.setValueAffectingTempoMap("curve", curveComboBox.currentValue);
    }
  }
}
