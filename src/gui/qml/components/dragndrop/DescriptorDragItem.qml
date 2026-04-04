// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Item {
  id: root

  property PluginDescriptor descriptor

  Drag.dragType: Drag.Automatic
  Drag.mimeData: root.descriptor ? {
    "application/x-plugin-descriptor": root.descriptor.serializeToString()
  } : ({})
  visible: false
}
