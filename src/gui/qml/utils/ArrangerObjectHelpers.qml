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

  /**
   * Processes items through loop segments and returns their visible positions
   *
   * @param loopRange The ArrangerObjectLoopRange to use for looping
   * @param regionLengthTicks Total length of region in ticks
   * @param contentWidth Width of content area in pixels
   * @param items Array of items to process, each with:
   *              - position.ticks: start position
   *              - length.ticks: item length
   * @param processCallback Function(item, absStart, absEnd) that returns processed result
   * @return Array of processed items for all visible loop segments
   */
  function processLoopedItems(loopRange: ArrangerObjectLoopRange, regionLengthTicks: real, contentWidth: real, items: var, processCallback: var): var {
    const results = []

    // Looped region: calculate notes for each loop segment
    const loopStartTicks = loopRange.loopStartPosition.ticks;
    const loopEndTicks = loopRange.loopEndPosition.ticks;
    const loopLengthTicks = Math.max(0.0, loopEndTicks - loopStartTicks);
    const clipStartTicks = loopRange.clipStartPosition.ticks;

    let loopSegmentVirtualStartTicks = clipStartTicks;
    let loopSegmentVirtualEndTicks = loopEndTicks;
    let loopSegmentAbsoluteStartTicks = 0;
    let loopSegmentAbsoluteEndTicks = loopEndTicks - clipStartTicks;
    if (loopSegmentAbsoluteEndTicks > regionLengthTicks) {
      const diff = loopSegmentAbsoluteEndTicks - regionLengthTicks;
      loopSegmentVirtualEndTicks -= diff;
      loopSegmentAbsoluteEndTicks -= diff;
    }

    while (loopSegmentAbsoluteStartTicks < regionLengthTicks) {
      // Add notes for this loop segment
      for (let i = 0; i < items.length; i++) {
        const item = items[i];
        const noteVirtualStartTicks = item.position.ticks;
        const noteVirtualEndTicks = noteVirtualStartTicks + item.bounds.length.ticks;

        // Only include notes that fall within the loop range
        if (noteVirtualStartTicks >= loopSegmentVirtualEndTicks || noteVirtualEndTicks <= loopSegmentVirtualStartTicks)
          continue;

        // Calculate position in current loop segment
        const noteAbsoluteStartTicks = Math.max(loopSegmentAbsoluteStartTicks, loopSegmentAbsoluteStartTicks + (noteVirtualStartTicks - loopSegmentVirtualStartTicks));
        const noteAbsoluteEndTicks = Math.min(loopSegmentAbsoluteEndTicks, loopSegmentAbsoluteStartTicks + (noteVirtualEndTicks - loopSegmentVirtualStartTicks));

        results.push(processCallback(item, noteAbsoluteStartTicks, noteAbsoluteEndTicks));
      }

      // Advance to next segment
      const currentLen = loopSegmentAbsoluteEndTicks - loopSegmentAbsoluteStartTicks;
      loopSegmentVirtualStartTicks = loopStartTicks;
      loopSegmentVirtualEndTicks = loopEndTicks;
      loopSegmentAbsoluteStartTicks += currentLen;
      loopSegmentAbsoluteEndTicks += loopLengthTicks;

      // Clip final segment to region bounds
      if (loopSegmentAbsoluteEndTicks > regionLengthTicks) {
        const diff = loopSegmentAbsoluteEndTicks - regionLengthTicks;
        loopSegmentVirtualEndTicks -= diff;
        loopSegmentAbsoluteEndTicks -= diff;
      }
    }

    return results;
  }
}
