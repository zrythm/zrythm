// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Item {
  id: root

  property PluginDescriptor descriptor

  // True while a drag start has been requested and is pending: the deferred
  // start consumes it, and cancelling the drag before it starts clears it
  property bool dragRequested: false

  Drag.dragType: Drag.Automatic
  Drag.mimeData: root.descriptor ? {
    "application/x-plugin-descriptor": root.descriptor.serializeToString()
  } : ({})
  visible: false
}
