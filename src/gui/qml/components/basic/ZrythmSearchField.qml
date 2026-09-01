// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle

/**
 * Search field filling gaps in the Qt template: placeholder text support,
 * a clear() function, and a focus check that follows the inner text input
 * (active focus lands on the content item, not this control).
 *
 * Key events reach attached Keys handlers here once the inner text input
 * declines them (e.g. Escape, Return, Up/Down, Tab), so callers can
 * repurpose them freely; Left/Right are consumed for caret movement and
 * cannot be intercepted.
 */
SearchField {
  id: control

  // Whether the text input holds active focus
  readonly property bool fieldActiveFocus: control.activeFocus || control.contentItem.activeFocus

  // Hint shown when the field is empty
  property string placeholderText: ""

  function clear() {
    control.text = "";
  }

  // Moves keyboard focus to the inner text input directly: the base
  // type's focusInEvent only forwards focus to the input for
  // Tab/Backtab/Shortcut/Other reasons
  function forceInputFocus() {
    control.contentItem.forceActiveFocus();
  }

  // The inner text input consumes Tab for the window's focus chain
  // before the control sees it, which breaks callers repurposing Tab;
  // disable it there and make this control tab-reachable instead (gaining
  // tab focus is forwarded to the text input internally)
  activeFocusOnTab: true

  // A plain binding is not possible here: the QML compiler rejects
  // properties not present on contentItem's static type (QQuickItem)
  Binding {
    property: "activeFocusOnTab"
    target: control.contentItem
    value: false
  }

  Binding {
    property: "placeholderText"
    target: control.contentItem
    value: control.placeholderText
  }
}
