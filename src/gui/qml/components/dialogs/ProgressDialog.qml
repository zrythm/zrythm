// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
  id: root

  property bool autoClose: true
  property bool autoReset: true
  property bool indeterminate: false
  property string labelText: ""
  property int maximum: 100
  property int minimum: 0
  readonly property real progress: (value - minimum) / (maximum - minimum)
  property int value: 0

  signal canceled

  function cancel() {
    root.canceled();
    root.close();
  }

  function reset() {
    root.value = minimum;
    if (autoClose) {
      root.close();
    }
  }

  closePolicy: Popup.NoAutoClose
  implicitWidth: 500
  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Cancel

  onRejected: root.cancel()

  // Update progress when value changes
  onValueChanged: {
    // Check if operation is complete
    if (value >= maximum && autoReset) {
      reset();
    }
  }

  ColumnLayout {
    anchors.centerIn: parent
    spacing: 12
    width: parent.width - 40

    Label {
      Layout.alignment: Qt.AlignHCenter
      font.bold: true
      text: root.labelText || "Processing..."
    }

    ProgressBar {
      Layout.fillWidth: true
      indeterminate: root.indeterminate
      value: root.progress
    }

    Label {
      Layout.alignment: Qt.AlignHCenter
      text: root.indeterminate ? "Please wait..." : Math.round(root.progress * 100) + "%"
    }
  }
}
