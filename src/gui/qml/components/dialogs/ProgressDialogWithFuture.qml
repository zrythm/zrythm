// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

ProgressDialog {
  id: root

  required property QFutureQmlWrapper future

  onCanceled: {
    root.future.cancel();
  }

  Connections {
    function onFinished() {
      console.log("Progress finished");
      if (root.autoClose) {
        root.close();
      }
    }

    target: root.future
  }

  Binding {
    property: "value"
    target: root
    value: root.future?.progressValue
    when: root.future
  }

  Binding {
    property: "labelText"
    target: root
    value: root.future?.progressText
    when: root.future && root.future.progressText.length > 0
  }

  Binding {
    property: "minimum"
    target: root
    value: root.future?.progressMinimum
    when: root.future
  }

  Binding {
    property: "maximum"
    target: root
    value: root.future?.progressMaximum
    when: root.future
  }
}
