// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

// Drag source for QDrag-based (Drag.Automatic) drags.
//
// Own instances outside list delegates (e.g. at the page or view root) so
// the drag source outlives the delegate being dragged.
Item {
  id: root

  visible: false
  Drag.dragType: Drag.Automatic
}
