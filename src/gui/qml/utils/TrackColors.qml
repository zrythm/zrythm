// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton
pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

/**
 * Track color resolution for QML: the reactive layer over the C++
 * TrackColorTable.
 *
 * Reads of a track's color state happen in this file so that bindings
 * calling effectiveColor() re-evaluate when the color changes.
 */
QtObject {
  /**
   * The color a track is drawn with: its user-set color, or the
   * mode-aware type default when none is set.
   */
  function effectiveColor(track: Track, dark: bool): color {
    return track.useColor
      ? track.color
      : TrackColorTable.defaultColor(track.type, dark);
  }

  /**
   * The default color of a track type in the given theme mode.
   */
  function defaultColor(trackType: int, dark: bool): color {
    return TrackColorTable.defaultColor(trackType, dark);
  }

  /**
   * The fixed hue of a track type, in degrees in [0, 360).
   */
  function hueOf(trackType: int): real {
    return TrackColorTable.hueOf(trackType);
  }

  /**
   * All track types in hue order, for the color picker.
   */
  function canonicalTypes(): var {
    return TrackColorTable.canonicalTypes();
  }
}
