// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle

T.ToolTip {
  id: control

  closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent
  delay: ZrythmTheme.toolTipDelay
  font.pointSize: ZrythmTheme.fontPointSize
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
  margins: ZrythmTheme.buttonPadding
  padding: ZrythmTheme.buttonPadding
  visible: parent && parent.hasOwnProperty("hovered") ? parent.hovered : false // qmllint disable missing-property
  x: parent ? (parent.width - implicitWidth) / 2 : 0
  // Initial placement only; recomputed on every open below
  y: -implicitHeight - 3

  // Above the control when it fits, otherwise below; when neither fits
  // (e.g. a toolbar-height scene), use the roomier side and let the popup
  // clamp into the window. Computed on each open: mapToItem() is a
  // function call and registers no binding dependency, so a binding
  // would keep the side choice of the control's previous position
  onAboutToShow: {
    if (!parent)
      return;
    const sceneY = parent.mapToItem(null, 0, 0).y;
    const windowHeight = parent.Window.window ? parent.Window.window.height : 0;
    const spaceAbove = sceneY;
    const spaceBelow = windowHeight - sceneY - parent.height;
    const needed = implicitHeight + 3;
    if (spaceAbove >= needed)
      y = -implicitHeight - 3;
    else if (spaceBelow >= needed)
      y = parent.height + 3;
    else
      y = spaceAbove >= spaceBelow ? -implicitHeight - 3 : parent.height + 3;
  }

  background: PopupBackgroundRect {
  }
  contentItem: Text {
    color: ZrythmTheme.colorPalette.toolTipText
    font: control.font
    text: control.text
    wrapMode: Text.Wrap
  }
}
