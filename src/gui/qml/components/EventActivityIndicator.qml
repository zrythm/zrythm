// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import ZrythmStyle

/**
 * Track-header event activity dot (DESIGN.md "Track event activity
 * indicators").
 *
 * A 4x18 item holding a centered 4x4 dot: an idle well using the standard
 * button fill and border role, and an accent-colored overlay that lights up
 * on pulse() with an immediate onset, stays lit for a fixed hold duration,
 * then fades out over the standard animation duration. A pulse during hold
 * or release relights it immediately and restarts the hold, so dense
 * activity stays lit instead of strobing. The item is decorative: it takes
 * no focus and is excluded from the accessibility tree.
 */
Item {
  id: root

  readonly property bool active: activeOverlay.opacity > 0

  // minimum lit time after the latest event batch, then opacity release
  readonly property int holdDurationMs: 120
  readonly property int releaseDurationMs: ZrythmTheme.animationDuration

  function pulse(): void {
    releaseAnimation.stop();
    activeOverlay.opacity = 1;
    holdTimer.restart();
  }

  function reset(): void {
    holdTimer.stop();
    releaseAnimation.stop();
    activeOverlay.opacity = 0;
  }

  // decorative: not focusable and hidden from the accessibility tree
  Accessible.ignored: true
  implicitHeight: ZrythmTheme.compactControlHeight
  width: 4

  Rectangle {
    id: idleWell

    objectName: "idleWell"

    anchors.centerIn: parent
    border.color: ZrythmTheme.colorPalette.mid
    border.width: 1
    color: ZrythmTheme.buttonBackgroundColor
    height: 4
    radius: 2
    width: 4
  }

  Rectangle {
    id: activeOverlay

    objectName: "activeOverlay"

    anchors.centerIn: parent
    color: ZrythmTheme.indicatorColor(ZrythmTheme.primaryColor)
    height: idleWell.height
    opacity: 0
    radius: idleWell.radius
    width: idleWell.width
  }

  NumberAnimation {
    id: releaseAnimation

    duration: root.releaseDurationMs
    easing.type: ZrythmTheme.animationEasingType
    property: "opacity"
    target: activeOverlay
    to: 0
  }

  Timer {
    id: holdTimer

    interval: root.holdDurationMs
    onTriggered: releaseAnimation.restart()
  }
}
