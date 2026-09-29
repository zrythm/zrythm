// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
  id: control

  alignment: Qt.AlignRight
  buttonLayout: T.DialogButtonBox.MacLayout
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  padding: 12
  spacing: 8

  // The accept button is the default; reject buttons (cancel and its
  // kin) never take the default, so a destructive accept leaves the
  // dialog without one.
  onStandardButtonsChanged: assignDefaultButton()

  // The MacLayout sort runs in component completion, after declarative
  // defaultButton assignments fire, so the view index is re-synced
  // once the row order is final.
  Component.onCompleted: {
    assignDefaultButton();
    syncCurrentIndex();
  }

  // The platform button press keys do not include Return or Enter, so
  // the box itself activates the default button on those keys. These
  // handlers only see the footer subtree; `Dialog.qml` owns a window
  // shortcut that covers the whole dialog.
  Keys.onEnterPressed: (event) => activateDefaultButton(event)
  Keys.onReturnPressed: (event) => activateDefaultButton(event)

  onCountChanged: {
    for (let i = 0; i < count; ++i)
      applyFillWidth(itemAt(i));
    syncCurrentIndex();
  }

  // Buttons share the row equally and fill it; a button never gets
  // narrower than its own label. Applied per item so manually declared
  // buttons fill the row alongside standard ones (the delegate never
  // wraps those). Heights follow the tallest button — a width binding
  // on an item opts it out of Qt's own height stretch.
  function applyFillWidth(item: Item): void {
    item.width = Qt.binding(() => Math.max(
      item.implicitWidth,
      (control.availableWidth - control.spacing * (control.count - 1)) / control.count));
    item.height = Qt.binding(() => control.implicitContentHeight);
  }

  function activateDefaultButton(event: KeyEvent): void {
    if (activateDefault())
      event.accepted = true;
  }

  // Clicks the default button; returns whether one was activated.
  function activateDefault(): bool {
    if (!defaultButton || !defaultButton.enabled)
      return false;

    defaultButton.clicked();
    return true;
  }

  onDefaultButtonChanged: syncCurrentIndex()

  function syncCurrentIndex(): void {
    buttonListView.currentIndex = -1;
    for (let i = 0; i < count; ++i) {
      if (itemAt(i) === defaultButton) {
        buttonListView.currentIndex = i;
        return;
      }
    }
  }

  function assignDefaultButton(): void {
    if (defaultButton) {
      for (let i = 0; i < count; ++i) {
        if (itemAt(i) === defaultButton)
          return;
      }

      // The incumbent is gone from the row (a standardButtons swap
      // removes the old buttons before this runs) — drop it and
      // reassign.
      defaultButton = null;
    }

    const acceptCandidates = [DialogButtonBox.Ok, DialogButtonBox.Save, DialogButtonBox.SaveAll, DialogButtonBox.Open, DialogButtonBox.Retry, DialogButtonBox.Ignore, DialogButtonBox.Yes, DialogButtonBox.YesToAll];
    for (const candidate of acceptCandidates) {
      const button = standardButton(candidate);
      if (button) {
        defaultButton = button;
        return;
      }
    }
  }

  background: Rectangle {
    color: control.palette.window
    height: parent.height - 2
    implicitHeight: 40
    width: parent.width - 2
    x: 1
    y: 1
  }
  contentItem: Item {
    ListView {
      id: buttonListView

      // Qt's updateFocus() only reaches a contentItem that is itself
      // an ItemView; the alignment wrapper hides the view, so the
      // template keeps the view's focus and currentIndex in sync with
      // the default button itself. Button focus and highlight remain
      // Qt's: the default button, else the first accept-role button.
      focus: control.defaultButton !== null
      height: parent.height
      interactive: false
      layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
      model: control.contentModel
      orientation: ListView.Horizontal
      spacing: control.spacing
      width: contentWidth
      x: control.mirrored ? 0 : parent.width - width
    }
  }
  delegate: Button {
  }
}
