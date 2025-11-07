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

  function getObjectEndInTimelineTicks(obj: ArrangerObject): real {
    let endTicks = getObjectEndTicks(obj);
    if (obj.parentObject) {
      endTicks += obj.parentObject.position.ticks;
    }
    return endTicks;
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

  function getObjectTimelinePositionInTicks(obj: ArrangerObject): real {
    if (obj.parentObject) {
      return obj.parentObject.position.ticks + obj.position.ticks;
    } else {
      return obj.position.ticks;
    }
  }

  function isRegion(obj: ArrangerObject): bool {
    return obj && (obj.type === ArrangerObject.MidiRegion || obj.type === ArrangerObject.AudioRegion || obj.type === ArrangerObject.ChordRegion || obj.type === ArrangerObject.AutomationRegion);
  }

  function setObjectEndFromTimelineTicks(obj: ArrangerObject, timelineTicks: real) {
    const bounds = getObjectBounds(obj);
    if (!bounds) {
      return;
    }

    if (obj.parentObject) {
      const localTicks = timelineTicks - obj.parentObject.position.ticks;
      bounds.length.ticks = localTicks - obj.position.ticks;
    } else {
      bounds.length.ticks = timelineTicks - obj.position.ticks;
    }
  }
}
