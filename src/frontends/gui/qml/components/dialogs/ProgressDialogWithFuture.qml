// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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

  // The dialog tracks progress only, so every outcome closes it
  function closeIfAutoClose() {
    if (root.autoClose) {
      root.close();
    }
  }

  Connections {
    function onCanceled() {
      root.closeIfAutoClose();
    }

    function onFailed(errorString: string) {
      root.closeIfAutoClose();
    }

    function onSucceeded() {
      root.closeIfAutoClose();
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
