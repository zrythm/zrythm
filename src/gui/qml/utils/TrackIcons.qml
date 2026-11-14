// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton
pragma ComponentBehavior: Bound

import Zrythm

/**
 * Track icon resolution for QML: maps a track's icon name (an optional
 * user-set string with an optional "<collection>-" prefix) to a resource
 * URL.
 */
QtObject {
  id: root
  /**
   * The URL of @p track's icon, resolved against the icon collection the
   * name prefixes, or the zrythm-dark collection by default.
   */
  function iconUrl(track: Track): url {
    return iconUrlFromName(track.icon);
  }

  /**
   * The URL of an icon name, resolved against the icon collection the
   * name prefixes, or the zrythm-dark collection by default.
   */
  function iconUrlFromName(iconName: string): url {
    if (iconName.startsWith("gnome-icon-library-"))
      return ResourceManager.getIconUrl("gnome-icon-library", iconName.substring(19) + ".svg");

    if (iconName.startsWith("fluentui-"))
      return ResourceManager.getIconUrl("fluentui", iconName.substring(9) + ".svg");

    if (iconName.startsWith("lorc-"))
      return ResourceManager.getIconUrl("lorc", iconName.substring(5) + ".svg");

    if (iconName.startsWith("jam-icons-"))
      return ResourceManager.getIconUrl("jam-icons", iconName.substring(10) + ".svg");

    return ResourceManager.getIconUrl("zrythm-dark", iconName + ".svg");
  }
}
