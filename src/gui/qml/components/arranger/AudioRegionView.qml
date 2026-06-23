// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm
import ZrythmStyle

RegionBaseView {
  id: root

  property bool isLanePart: false
  required property TrackLane lane
  readonly property AudioRegion region: arrangerObject as AudioRegion
  required property TempoMap tempoMap

  headerExtra: Item {
    id: badgeContainer

    readonly property double endTick: startTick + root.region.bounds.timelineLengthTicks
    property bool hasTempoChanges: false
    property bool isStretching: false
    readonly property double startTick: root.region.position.ticks
    property double tempoAtStart: 0

    function computeHasTempoChanges() {
      const events = root.tempoMap.tempoEvents;
      const startTick = Math.round(badgeContainer.startTick);
      const endTick = Math.round(badgeContainer.endTick);
      let prevBpm = 0;
      let activeCurve = 0;
      for (let i = 0; i < events.length; i++) {
        const ev = events[i];
        if (ev.tick <= startTick) {
          activeCurve = ev.curve;
          prevBpm = ev.bpm;
        } else if (ev.tick < endTick) {
          if (ev.curve === 1 || Math.abs(ev.bpm - prevBpm) > 0.5) {
            return true;
          }
          prevBpm = ev.bpm;
        }
      }
      if (activeCurve === 1) {
        return true;
      }
      return false;
    }

    function updateState() {
      if (!root.region.effectivelyInMusicalMode || root.region.sourceBpm <= 0) {
        badgeContainer.isStretching = false;
        badgeContainer.hasTempoChanges = false;
        return;
      }

      badgeContainer.tempoAtStart = root.tempoMap.tempoAtTick(Math.round(badgeContainer.startTick));
      badgeContainer.hasTempoChanges = computeHasTempoChanges();

      if (badgeContainer.hasTempoChanges) {
        badgeContainer.isStretching = true;
      } else {
        badgeContainer.isStretching = Math.abs(root.region.sourceBpm - badgeContainer.tempoAtStart) > 0.5;
      }
    }

    height: 14
    visible: root.region.effectivelyInMusicalMode && root.region.sourceBpm > 0 && root.headerDetailLevel < 2
    width: visible ? badgeText.implicitWidth + 4 : 0

    Component.onCompleted: badgeContainer.updateState()

    Rectangle {
      id: badgeBackground

      anchors.fill: parent
      color: ZrythmTheme.colorPalette.highlight
      radius: 3
      visible: badgeContainer.isStretching
    }

    Connections {
      function onEffectivelyInMusicalModeChanged() {
        badgeContainer.updateState();
      }

      target: root.region
    }

    Connections {
      function onTempoEventsChanged() {
        badgeContainer.updateState();
      }

      target: root.tempoMap
    }

    Connections {
      function onPositionChanged() {
        badgeContainer.updateState();
      }

      target: root.region.position
    }

    Connections {
      function onPositionChanged() {
        badgeContainer.updateState();
      }

      target: root.region.bounds.length
    }

    Text {
      id: badgeText

      color: badgeContainer.isStretching ? ZrythmTheme.colorPalette.highlightedText : Qt.alpha(root.palette.buttonText, 0.6)
      font.bold: true
      font.family: ZrythmTheme.fontFamily
      font.pixelSize: 10
      text: {
        if (root.headerDetailLevel > 0 || !badgeContainer.isStretching) {
          return "♪";
        }
        const src = root.region.sourceBpm.toFixed(0);
        if (badgeContainer.hasTempoChanges) {
          return "♪ " + src + "→var";
        }
        return "♪ " + src + "→" + badgeContainer.tempoAtStart.toFixed(0);
      }

      anchors {
        right: parent.right
        rightMargin: 2
        verticalCenter: parent.verticalCenter
      }
    }
  }
  regionContent: AudioRegionContent {
    contentHeight: root.contentHeight
    contentWidth: root.contentWidth
    region: root.region
    tempoMap: root.tempoMap
  }
}
