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
 * Presents the notifications of a window: toasts for Info/Success/Warning/
 * Error, and the criticalNotification signal for Critical notifications,
 * which the window shows as a modal dialog.
 *
 * At most @ref maxVisible toasts are shown at once; further events queue in
 * arrival order and appear as slots free up. Events sharing severity, title
 * and context tag coalesce into the existing toast, which counts the
 * occurrences it absorbed. Explicitly dismissing a toast (close button,
 * click, Escape or its action) acknowledges every occurrence it absorbed;
 * expiry does not acknowledge anything.
 */
Item {
  id: root

  property NotificationCenter notificationCenter: null

  /** Auto-dismissal timeouts. */
  property int infoDurationMs: 5000
  property int successDurationMs: 5000
  property int warningDurationMs: 10000
  property int errorDurationMs: 30000

  readonly property int maxVisible: 3

  readonly property alias visibleCount: visibleModel.count
  readonly property int pendingCount: queuedToasts.length

  signal criticalNotification(Notification notification)
  signal actionTriggered(string actionId)

  anchors.top: parent.top
  anchors.topMargin: 8
  height: toastColumn.implicitHeight
  width: 360
  x: parent ? (GlobalState.application?.layoutDirection === Qt.RightToLeft ? 16 : parent.width - width - 16) : 0
  z: 1000

  // Queued toasts as { notification, occurrences } wrappers; they have no
  // delegates until promoted
  property var queuedToasts: []

  function present(notification: Notification) {
    if (notification.severity === Notification.Critical) {
      // Presenting a critical notification acknowledges it: the
      // hosting window always shows it as a modal dialog
      notification.acknowledged = true;
      criticalNotification(notification);
      return;
    }

    for (let i = 0; i < visibleModel.count; ++i) {
      if (notification.coalescesWith(visibleModel.get(i).notification)) {
        (toastRepeater.itemAt(i) as Toast)?.absorb(notification);
        return;
      }
    }
    for (let i = 0; i < queuedToasts.length; ++i) {
      if (notification.coalescesWith(queuedToasts[i].notification)) {
        queuedToasts[i].occurrences =
          queuedToasts[i].occurrences.concat([notification]);
        return;
      }
    }

    if (visibleModel.count < root.maxVisible) {
      visibleModel.insert(0, {"notification": notification});
    } else {
      queuedToasts = queuedToasts.concat([
        {"notification": notification, "occurrences": [notification]}
      ]);
    }
  }

  function topmostCount(): int {
    const topmost = toastRepeater.itemAt(0) as Toast;
    return topmost ? topmost.count : 0;
  }

  function dismissTopmost() {
    const topmost = toastRepeater.itemAt(0) as Toast;
    topmost?.dismiss(true);
  }

  function promoteNext() {
    if (visibleModel.count >= root.maxVisible || queuedToasts.length === 0)
      return;

    const next = queuedToasts[0];
    visibleModel.insert(
      visibleModel.count, {"notification": next.notification});
    queuedToasts = queuedToasts.slice(1);
    (toastRepeater.itemAt(visibleModel.count - 1) as Toast)
      ?.setOccurrences(next.occurrences);
  }

  // Critical notifications posted before this area existed (e.g. during
  // startup, before the window is created) are presented on creation
  Component.onCompleted: {
    if (root.Window.window !== null && root.Window.window.visible) {
      root.notificationCenter?.unacknowledgedCriticals().forEach(
        (notification) => root.present(notification));
    }
  }

  Connections {
    enabled: root.notificationCenter !== null && root.Window.window !== null && root.Window.window.visible

    function onNotificationPosted(notification: Notification) {
      root.present(notification);
    }

    target: root.notificationCenter
  }

  // Escape dismisses the topmost toast when no popup, menu or dialog holds
  // the focus (items inside popups are parented into the window's overlay)
  Shortcut {
    enabled: root.visibleCount > 0 && root.Window.window !== null && root.Window.window.activeFocusItem !== null && !root.popupHasActiveFocus

    onActivated: root.dismissTopmost()
    sequences: [StandardKey.Cancel]
  }

  readonly property bool popupHasActiveFocus: {
    const overlay = root.Overlay.overlay;
    let item = root.Window.window?.activeFocusItem ?? null;
    while (item !== null && item !== undefined) {
      if (item === overlay)
        return true;
      item = item.parent;
    }
    return false;
  }

  ListModel {
    id: visibleModel
  }

  Column {
    id: toastColumn
    spacing: 8
    width: root.width

    Repeater {
      id: toastRepeater
      model: visibleModel

      delegate: Toast {
        id: toastDelegate

        onActionRequested: (actionId) => root.actionTriggered(actionId)

        onDismissed: (userDismissed) => {
          if (userDismissed) {
            for (const notification of toastDelegate.occurrences)
              notification.acknowledged = true;
          }
          visibleModel.remove(toastDelegate.index);
          root.promoteNext();
        }
      }
    }
  }

  component Toast: Rectangle {
    id: toast

    required property int index
    required property Notification notification

    /** The occurrences this toast absorbed, oldest first. */
    property var occurrences: []
    readonly property int count: occurrences.length

    readonly property color severityColor: {
      if (toast.notification.severity === Notification.Info)
        return toast.palette.windowText;
      if (toast.notification.severity === Notification.Success)
        return ZrythmTheme.successColor;
      if (toast.notification.severity === Notification.Warning)
        return ZrythmTheme.warningColor;
      return ZrythmTheme.errorColor;
    }
    readonly property color surfaceColor: {
      if (toast.notification.severity === Notification.Info)
        return ZrythmTheme.notificationSurfaceInfoColor;
      if (toast.notification.severity === Notification.Success)
        return ZrythmTheme.notificationSurfaceSuccessColor;
      if (toast.notification.severity === Notification.Warning)
        return ZrythmTheme.notificationSurfaceWarningColor;
      return ZrythmTheme.notificationSurfaceErrorColor;
    }
    readonly property string iconName: {
      if (toast.notification.severity === Notification.Info)
        return "info-outline-symbolic.svg";
      if (toast.notification.severity === Notification.Success)
        return "check-round-outline-symbolic.svg";
      if (toast.notification.severity === Notification.Warning)
        return "warning-outline-symbolic.svg";
      return "cross-small-circle-outline-symbolic.svg";
    }
    readonly property int durationMs: {
      if (toast.notification.severity === Notification.Info)
        return root.infoDurationMs;
      if (toast.notification.severity === Notification.Success)
        return root.successDurationMs;
      if (toast.notification.severity === Notification.Warning)
        return root.warningDurationMs;
      if (toast.notification.severity === Notification.Error)
        return root.errorDurationMs;
      return 0;
    }

    signal dismissed(bool userDismissed)
    signal actionRequested(string actionId)

    property bool closing: false
    property bool userDismissed: false
    property real remainingMs: 0
    property real startedAtMs: 0

    function dismiss(userInitiated: bool) {
      if (toast.closing)
        return;
      toast.userDismissed = userInitiated;
      toast.closing = true;
      exitAnimation.start();
    }

    function absorb(notification: Notification) {
      toast.occurrences = toast.occurrences.concat([notification]);
    }

    function setOccurrences(occurrences: var) {
      toast.occurrences = occurrences;
    }

    function restartDismissTimer() {
      if (toast.durationMs <= 0) {
        dismissTimer.stop();
        return;
      }
      toast.remainingMs = toast.durationMs;
      if (!toast.paused) {
        toast.startedAtMs = Date.now();
        dismissTimer.interval = toast.durationMs;
        dismissTimer.restart();
      }
    }

    Accessible.description: toast.notification.detail
    Accessible.name: toast.notification.title
    Accessible.role: Accessible.Notification
    border.color: toast.palette.mid
    border.width: 1
    color: toast.surfaceColor
    height: content.implicitHeight + 24 // 12 px top and bottom margins
    layer.enabled: true
    layer.effect: DropShadowEffect {
    }
    radius: ZrythmTheme.textFieldRadius
    width: root.width

    Component.onCompleted: {
      toast.occurrences = [toast.notification];
      enterAnimation.start();
      restartDismissTimer();
    }
    onCountChanged: restartDismissTimer()

    // Hover or keyboard focus on the close button pauses the dismissal
    // timer; unpausing resumes with the remaining time
    readonly property bool paused: hoverHandler.hovered || closeButton.activeFocus
    onPausedChanged: {
      if (toast.durationMs <= 0 || toast.closing)
        return;
      if (toast.paused) {
        toast.remainingMs = Math.max(0, toast.durationMs - (Date.now() - toast.startedAtMs));
        dismissTimer.stop();
      } else if (toast.remainingMs > 0) {
        dismissTimer.interval = Math.ceil(toast.remainingMs);
        toast.startedAtMs = Date.now();
        dismissTimer.start();
      }
    }

    Timer {
      id: dismissTimer

      onTriggered: toast.dismiss(false)
    }

    ParallelAnimation {
      id: enterAnimation

      NumberAnimation {
        duration: ZrythmTheme.animationDuration
        easing.type: ZrythmTheme.animationEasingType
        from: 0
        property: "opacity"
        target: toast
        to: 1
      }
      NumberAnimation {
        duration: ZrythmTheme.animationDuration
        easing.type: ZrythmTheme.animationEasingType
        from: -8
        property: "y"
        target: toast
        to: 0
      }
    }

      NumberAnimation {
        id: exitAnimation

        duration: ZrythmTheme.animationDuration
        from: 1
        onStopped: toast.dismissed(toast.userDismissed)
        property: "opacity"
        target: toast
        to: 0
      }

    MouseArea {
      anchors.fill: parent
      onClicked: toast.dismiss(true)
    }

    HoverHandler {
      id: hoverHandler
    }

    ColumnLayout {
      id: content
      anchors.fill: parent
      anchors.margins: 12
      spacing: 4

      RowLayout {
        spacing: 6
        Layout.fillWidth: true

        IconImage {
          Layout.preferredHeight: 16
          Layout.preferredWidth: 16
          color: toast.severityColor
          source: ResourceManager.getIconUrl("gnome-icon-library", toast.iconName)
        }

        Text {
          color: toast.palette.windowText
          elide: Text.ElideRight
          font: ZrythmTheme.semiBoldTextFont
          Layout.fillWidth: true
          text: toast.notification.title
        }

        Rectangle {
          color: toast.palette.mid
          implicitHeight: chipLabel.implicitHeight + 4
          implicitWidth: chipLabel.implicitWidth + 6
          radius: height / 2
          visible: toast.count > 1

          Text {
            id: chipLabel
            anchors.centerIn: parent
            color: toast.palette.windowText
            font: ZrythmTheme.smallTextFont
            opacity: 0.7
            text: "×" + toast.count
          }
        }

        Button {
          visible: toast.notification.actionLabel !== ""
          implicitHeight: 24

          palette.buttonText: toast.palette.link

          onClicked: {
            toast.actionRequested(toast.notification.actionId);
            toast.dismiss(true);
          }
          text: toast.notification.actionLabel
        }

        ToolButton {
          id: closeButton
          activeFocusOnTab: true
          implicitWidth: 24
          implicitHeight: 24
          icon.color: toast.palette.windowText
          icon.width: 16
          icon.height: 16
          icon.source: ResourceManager.getIconUrl("gnome-icon-library", "cross-small-symbolic.svg")
          opacity: closeButton.hovered || closeButton.activeFocus ? 1.0 : 0.7

          onClicked: toast.dismiss(true)
        }
      }

      Text {
        color: toast.palette.windowText
        elide: Text.ElideRight
        font: ZrythmTheme.fadedTextFont
        Layout.fillWidth: true
        maximumLineCount: 3
        opacity: 0.7
        text: toast.notification.detail
        visible: toast.notification.detail !== ""
        wrapMode: Text.WordWrap
      }
    }
  }
}
