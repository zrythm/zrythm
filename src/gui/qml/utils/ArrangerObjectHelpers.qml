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
      return obj.position.ticks + bounds.timelineLengthTicks;
    } else if (obj.type === ArrangerObject.ChordObject && obj.parentObject) {
      // Chord objects have no bounds — derive end from the next distinct
      // chord position in the region, or the region end if this is the last.
      const region = obj.parentObject;
      const myPos = obj.position.ticks;
      let nextPos = -1;
      const chordObjects = region.chordObjects;
      for (let i = 0; i < chordObjects.rowCount(); i++) {
        const other = chordObjects.data(chordObjects.index(i, 0), ArrangerObjectListModel.ArrangerObjectPtrRole);
        const otherPos = other.position.ticks;
        if (otherPos > myPos + 1e-9) {
          if (nextPos < 0 || otherPos < nextPos)
            nextPos = otherPos;
        }
      }
      if (nextPos >= 0)
        return nextPos;
      const regionBounds = region.bounds;
      if (regionBounds)
        return regionBounds.timelineLengthTicks;
      return myPos;
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

  function objectLengthFromTimelineEndTicks(obj: ArrangerObject, timelineTicks: real): real {
    const parentOffset = obj.parentObject?.position.ticks ?? 0;
    return timelineTicks - parentOffset - obj.position.ticks;
  }

  function setObjectEndFromTimelineTicks(obj: ArrangerObject, timelineTicks: real) {
    const bounds = getObjectBounds(obj);
    if (!bounds) {
      return;
    }
    bounds.length.ticks = objectLengthFromTimelineEndTicks(obj, timelineTicks);
  }
}
