// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Dialog {
  id: control

  implicitHeight: Math.max(
    implicitBackgroundHeight + topInset + bottomInset,
    contentHeight + topPadding + bottomPadding
      + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
      + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))
  implicitWidth: Math.max(
    implicitBackgroundWidth + leftInset + rightInset,
    contentWidth + leftPadding + rightPadding, implicitHeaderWidth,
    implicitFooterWidth)
  padding: 12

  T.Overlay.modal: Rectangle {
    Behavior on opacity {
      animation: ZrythmTheme.propertyAnimation
    }
    color: Color.transparent("black", 0.35)
  }
  T.Overlay.modeless: Rectangle {
    color: Color.transparent(control.palette.shadow, 0.12)
  }

  // The flat client fill inside the dialog frame. Most app dialogs open
  // as native windows (the platform provides the title bar and frame);
  // in-scene dialogs render the same fill.
  background: Rectangle {
    color: control.palette.window
  }

  // Return and Enter activate the default button from anywhere in the
  // dialog; multi-line editors keep Return for newlines. The button
  // box's own key handlers only see the footer subtree.
  Shortcut {
    autoRepeat: false
    context: Qt.WindowShortcut
    enabled: {
      if (!control.visible)
        return false;

      const box = control.footer as DialogButtonBox;
      if (!box?.defaultButton || !box.defaultButton.enabled)
        return false;

      const contentItem = control.contentItem;
      if (!contentItem)
        return false;

      const focusItem = contentItem.Window.activeFocusItem;
      return !(focusItem instanceof T.TextArea)
        && !(focusItem instanceof TextEdit);
    }
    sequences: ["Return", "Enter"]

    onActivated: {
      const box = control.footer as DialogButtonBox;
      if (box?.defaultButton?.enabled)
        box.defaultButton.clicked();
    }
  }

  footer: DialogButtonBox {
    visible: count > 0
  }
  header: Label {
    elide: Label.ElideRight
    font: ZrythmTheme.semiBoldTextFont
    padding: 12
    text: control.title
    visible: parent?.parent === Overlay.overlay && control.title
  }

  onOpened: {
    // Focus the first text field so typing starts immediately; dialogs
    // without one keep focus on the default button. A dialog with
    // neither — a destructive accept leaves no default — focuses its
    // content, keeping focus off the button row.
    const field = findFirstTextField(contentItem);
    if (field) {
      field.forceActiveFocus();
      return;
    }

    const box = footer as DialogButtonBox;
    if (!box?.defaultButton)
      contentItem.forceActiveFocus();
  }

  function findFirstTextField(item) {
    for (let i = 0; i < item.children.length; ++i) {
      const child = item.children[i];
      // Skip whole branches, not just leaf matches — a hidden container
      // can hold a field whose own visible flag is still true.
      if (!child.visible || !child.enabled)
        continue;

      if (child.text !== undefined && child.cursorPosition !== undefined)
        return child;

      const nested = findFirstTextField(child);
      if (nested)
        return nested;
    }
    return null;
  }
}
