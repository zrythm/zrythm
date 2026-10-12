// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Controls
import ZrythmStyle

// Button for destructive actions (delete, discard). Takes the shared
// button style with the error hue as resting ink; when checkable and
// checked, the error fill with polarity text (DESIGN.md "Buttons",
// Destructive variant).
Button {
  palette.accent: ZrythmTheme.errorColor
  palette.buttonText: ZrythmTheme.errorTextColor
}
