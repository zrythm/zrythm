// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton
pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle

/**
 * Color of each audio transform entry kind, so every view that draws
 * entry kinds (stack panel, waveform highlights) uses one palette.
 *
 * Colors come from the theme's accent-derived palette
 * (ZrythmTheme.distinctColor); the table below maps each kind id to
 * its fixed slot in that palette, in the order of the
 * AudioFunction::Kind variant alternatives.
 */
QtObject {
  id: root

  readonly property var kindOrder: [
    "invert-polarity",
    "reverse",
    "normalize-peak",
    "normalize-rms",
    "fade-in",
    "fade-out",
  ]

  function kindColor (kindId : string) : color {
    const index = root.kindOrder.indexOf (kindId);
    const slot = index >= 0 ? index : 0;
    return ZrythmTheme.distinctColor (slot, root.kindOrder.length);
  }
}
