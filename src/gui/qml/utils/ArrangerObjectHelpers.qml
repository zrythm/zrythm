// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton

import QtQuick
import Zrythm

QtObject {
  function getObjectBounds(obj: var): ArrangerObjectBounds {
    if (obj.bounds) {
      return obj.bounds;
    } else {
      return null;
    }
  }

  function getObjectEndTicks(obj: ArrangerObject): real {
    let bounds = getObjectBounds(obj);
    if (bounds) {
      return obj.position.ticks + bounds.length.ticks;
    } else {
      return obj.position.ticks;
    }
  }

  function getObjectName(obj: var): ArrangerObjectName {
    if (obj.name) {
      return obj.name;
    } else {
      return null;
    }
  }

  function isRegion(obj: ArrangerObject): bool {
    return obj && (obj.type === ArrangerObject.MidiRegion || obj.type === ArrangerObject.AudioRegion || obj.type === ArrangerObject.ChordRegion || obj.type === ArrangerObject.AutomationRegion);
  }
}
