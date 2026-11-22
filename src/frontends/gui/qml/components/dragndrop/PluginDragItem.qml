// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

AutoDragItem {
  id: root

  property var selectedPlugins: []
  property PluginGroup sourceGroup
  property Track sourceTrack

  Drag.mimeData: {
    return {
      "application/x-plugin-list": "move"
    };
  }
}
