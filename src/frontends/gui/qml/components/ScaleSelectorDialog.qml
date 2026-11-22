// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Dialog {
  id: root

  required property MusicalScale musicalScale

  signal scaleSelected(int rootKey, int scaleType)

  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Ok | Dialog.Cancel
  title: qsTr("Edit Scale")

  property int tempRootKey: root.musicalScale.rootKey
  property int tempScaleType: root.musicalScale.scaleType

  contentItem: ColumnLayout {
    spacing: ZrythmTheme.buttonPadding

    Label {
      font.bold: true
      text: qsTr("Root Key:")
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 2
      uniformCellSizes: true

      Repeater {
        model: 12

        AbstractButton {
          id: rootKeyBtn

          required property int index
          readonly property bool isCurrent: index === root.tempRootKey

          Layout.fillWidth: true
          Layout.preferredHeight: 32

          background: Rectangle {
            color: rootKeyBtn.isCurrent ? root.palette.highlight : root.palette.button
            radius: 3
            border.width: 1
            border.color: root.palette.mid
          }

          contentItem: Label {
            color: rootKeyBtn.isCurrent ? root.palette.highlightedText : root.palette.text
            font.bold: rootKeyBtn.isCurrent
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: root.musicalScale.noteToString(rootKeyBtn.index)
          }

          onClicked: root.tempRootKey = index
        }
      }
    }

    Label {
      font.bold: true
      text: qsTr("Scale:")
    }

    ComboBox {
      id: commonScaleCombo

      Layout.fillWidth: true
      textRole: "display"
      valueRole: "value"
      model: root.musicalScale.availableScaleTypes()

      onActivated: function (index) {
        root.tempScaleType = currentValue;
        exoticScaleCombo.currentIndex = -1;
      }
    }

    GroupBox {
      id: exoticGroup

      Layout.fillWidth: true
      title: qsTr("Exotic Scales")

      ComboBox {
        id: exoticScaleCombo

        width: exoticGroup.availableWidth
        textRole: "display"
        valueRole: "value"
        model: root.musicalScale.availableScaleTypesExotic()

        onActivated: function (index) {
          root.tempScaleType = currentValue;
          commonScaleCombo.currentIndex = -1;
        }
      }
    }

    Label {
      font.bold: true
      text: qsTr("Notes in Scale:")
    }

    RowLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 4

      Repeater {
        model: 12

        Rectangle {
          id: noteDot

          required property int index

          Layout.preferredWidth: 20
          Layout.preferredHeight: 20
          radius: 10
          color: root.musicalScale.scaleContainsNote(
            root.tempScaleType,
            root.tempRootKey,
            index) ? root.palette.highlight : root.palette.mid

          Label {
            anchors.centerIn: parent
            color: noteDot.color === root.palette.highlight ? root.palette.highlightedText : root.palette.text
            font.pixelSize: 8
            text: root.musicalScale.noteToString(noteDot.index)
          }
        }
      }
    }
  }

  onAboutToShow: {
    root.tempRootKey = root.musicalScale.rootKey;
    root.tempScaleType = root.musicalScale.scaleType;
    syncComboBoxes();
  }

  onAccepted: {
    root.scaleSelected(root.tempRootKey, root.tempScaleType);
  }

  function syncComboBoxes() {
    const typeVal = root.tempScaleType;
    for (let i = 0; i < commonScaleCombo.count; ++i) {
      if (commonScaleCombo.model[i].value === typeVal) {
        commonScaleCombo.currentIndex = i;
        exoticScaleCombo.currentIndex = -1;
        return;
      }
    }
    for (let i = 0; i < exoticScaleCombo.count; ++i) {
      if (exoticScaleCombo.model[i].value === typeVal) {
        exoticScaleCombo.currentIndex = i;
        commonScaleCombo.currentIndex = -1;
        return;
      }
    }
    commonScaleCombo.currentIndex = -1;
    exoticScaleCombo.currentIndex = -1;
  }
}
