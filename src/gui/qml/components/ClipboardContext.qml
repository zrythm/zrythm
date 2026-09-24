// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQml

/// The clipboard operations a focused view provides to the window's
/// clipboard shortcuts (see ProjectWindow's activeClipboardContext).
///
/// A view instantiates this as its clipboardContext, handles the four
/// request signals and the two gating properties, and assigns itself
/// to the window's activeClipboardContext while focused. The
/// properties are required so a view cannot provide a partial
/// implementation.
QtObject {
  id: root

  /// Whether copy/cut/duplicate can act on the current selection.
  required property bool canCopy

  /// Whether the clipboard holds content this context can paste.
  required property bool canPaste

  /// Emitted to copy the current selection to the clipboard.
  signal copyRequested()

  /// Emitted to copy the current selection to the clipboard, then
  /// remove it.
  signal cutRequested()

  /// Emitted to paste the clipboard contents at this context's paste
  /// position.
  signal pasteRequested()

  /// Emitted to duplicate the current selection without touching the
  /// clipboard.
  signal duplicateRequested()
}
