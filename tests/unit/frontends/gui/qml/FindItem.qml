// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

// Shared depth-first lookup by objectName for QML tests.
QtObject {
  function byName(item: Item, name: string): Item {
    if (item.objectName === name)
      return item;
    for (let i = 0; i < item.children.length; i++) {
      const found = byName(item.children[i], name);
      if (found !== null)
        return found;
    }
    return null;
  }
}
