// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Popover listing the retained notification history: severity glyph,
 * title, elided detail, relative timestamp and ×N chip per row, with a
 * Clear All action.
 *
 * Opening the popover marks every retained notification as
 * acknowledged, and so does closing it (notifications that arrived
 * while it was open are covered by the close). Row timestamps refresh
 * while the popover is open.
 */
Popup {
  id: root

  required property NotificationCenter notificationCenter

  /** Reference time for the relative row timestamps. */
  property real nowMs: Date.now()

  implicitWidth: 360

  onClosed: root.notificationCenter?.acknowledgeAll()
  onOpened: {
    root.nowMs = Date.now();
    root.notificationCenter?.acknowledgeAll();
  }

  function relativeTimestamp(timestamp: date): string {
    const seconds = Math.floor((root.nowMs - timestamp.getTime()) / 1000);
    if (seconds < 10)
      return qsTr("just now");
    if (seconds < 60)
      return qsTr("%1 s ago").arg(seconds);
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60)
      return qsTr("%1 min ago").arg(minutes);
    const hours = Math.floor(minutes / 60);
    if (hours < 24)
      return qsTr("%1 h ago").arg(hours);
    return timestamp.toLocaleString(Qt.locale(), Locale.ShortFormat);
  }

  Timer {
    interval: 1000
    repeat: true
    running: root.opened

    onTriggered: root.nowMs = Date.now()
  }

  contentItem: ColumnLayout {
    spacing: 8

    ListView {
      id: historyView

      clip: true
      implicitHeight: Math.min(contentHeight, 360)
      Layout.fillWidth: true
      model: root.notificationCenter?.history ?? null

      ScrollIndicator.vertical: ScrollIndicator {
      }

      delegate: Item {
        id: entry

        required property int severity
        required property string title
        required property string detail
        required property date timestamp

        implicitHeight: entryLayout.implicitHeight + 16
        width: historyView.width

        readonly property color severityColor: {
          if (entry.severity === Notification.Info)
            return entry.palette.windowText;
          if (entry.severity === Notification.Success)
            return ZrythmTheme.successColor;
          if (entry.severity === Notification.Warning)
            return ZrythmTheme.warningColor;
          return ZrythmTheme.errorColor;
        }
        readonly property string iconName: {
          if (entry.severity === Notification.Info)
            return "info-outline-symbolic.svg";
          if (entry.severity === Notification.Success)
            return "check-round-outline-symbolic.svg";
          if (entry.severity === Notification.Warning)
            return "warning-outline-symbolic.svg";
          return "cross-small-circle-outline-symbolic.svg";
        }

        ColumnLayout {
          id: entryLayout

          anchors.fill: parent
          anchors.leftMargin: 8
          anchors.rightMargin: 8
          anchors.topMargin: 8
          anchors.bottomMargin: 8
          spacing: 2

          RowLayout {
            spacing: 6

            IconImage {
              Layout.preferredHeight: 16
              Layout.preferredWidth: 16
              color: entry.severityColor
              source: ResourceManager.getIconUrl("gnome-icon-library", entry.iconName)
            }

            Text {
              color: entry.palette.windowText
              elide: Text.ElideRight
              font: ZrythmTheme.semiBoldTextFont
              Layout.fillWidth: true
              text: entry.title
            }

            Text {
              color: entry.palette.windowText
              font: ZrythmTheme.smallTextFont
              opacity: 0.7
              text: root.relativeTimestamp(entry.timestamp)
            }
          }

          Text {
            color: entry.palette.windowText
            elide: Text.ElideRight
            font: ZrythmTheme.fadedTextFont
            Layout.fillWidth: true
            maximumLineCount: 2
            opacity: 0.7
            text: entry.detail
            visible: entry.detail !== ""
            wrapMode: Text.WordWrap
          }
        }
      }

      Text {
        anchors.centerIn: parent
        color: parent.palette.windowText
        font: ZrythmTheme.fadedTextFont
        opacity: 0.7
        text: qsTr("No Notifications")
        visible: historyView.count === 0
      }
    }

    RowLayout {
      visible: historyView.count > 0

      Item {
        Layout.fillWidth: true
      }

      Button {
        flat: true

        onClicked: root.notificationCenter?.clearHistory()
        text: qsTr("Clear All")
      }
    }
  }
}
