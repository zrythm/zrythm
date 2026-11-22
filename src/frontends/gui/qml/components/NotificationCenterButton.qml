// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

/**
 * Toolbar button that opens the notification history popover.
 *
 * The badge counts the unacknowledged warning and error notifications,
 * capped at "9+", and is filled with the color of the highest
 * unacknowledged severity.
 */
ToolButton {
  id: root

  required property NotificationCenter notificationCenter

  readonly property alias badgeText: badgeLabel.text
  readonly property alias badgeVisible: badge.visible
  readonly property NotificationCenterPopover popover: NotificationCenterPopover {
    id: historyPopup

    notificationCenter: root.notificationCenter
    x: GlobalState.application?.layoutDirection === Qt.RightToLeft ? 0 : root.width - width
    y: root.height
  }

  icon.source: ResourceManager.getIconUrl("gnome-icon-library", "bell-outline-symbolic.svg")

  onClicked: historyPopup.visible ? historyPopup.close() : historyPopup.open()

  Accessible.name: qsTr("Notifications")

  ToolTip {
    text: qsTr("Notifications")
  }

  Rectangle {
    id: badge

    readonly property int count:
      root.notificationCenter?.history.unacknowledgedAttentionCount ?? 0

    // Layers the badge above the button's icon (the contentItem)
    z: 1
    anchors.right: GlobalState.application?.layoutDirection === Qt.RightToLeft ? root.left : root.right
    anchors.top: root.top
    color:
      root.notificationCenter?.history.highestUnacknowledgedAttentionSeverity
        === Notification.Error ? ZrythmTheme.errorColor : ZrythmTheme.warningColor
    implicitHeight: 14
    implicitWidth: Math.max(badgeLabel.implicitWidth + 8, 14)
    radius: height / 2
    visible: badge.count > 0

    Text {
      id: badgeLabel

      anchors.centerIn: parent
      color: ZrythmTheme.polarityTextColor(badge.color)
      font: ZrythmTheme.smallTextFont
      text: badge.count > 9 ? "9+" : String(badge.count)
    }
  }
}
