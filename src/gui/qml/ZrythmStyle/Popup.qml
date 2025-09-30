// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle

T.Popup {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
  padding: 12
  popupType: Popup.Native

  T.Overlay.modal: Rectangle {
    color: {
      const c = control.palette.shadow;
      return Qt.rgba(c.r, c.g, c.b, 0.5);
    }
  }
  T.Overlay.modeless: Rectangle {
    color: {
      const c = control.palette.shadow;
      return Qt.rgba(c.r, c.g, c.b, 0.12);
    }
  }
  background: PopupBackgroundRect {
  }
}
