// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Controls for a single channel send, laid out as one dense row:
 * enable dot (destination color), amount bar (destination color and
 * name; drag adjusts the amount, click opens the destination menu),
 * fader tap-point chip (lit = post-fader, dim = pre-fader) and remove.
 */
RowLayout {
  id: root

  required property ChannelSendRouting channelSendRouting
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property ChannelSend send
  required property Track track
  required property UndoStack undoStack

  property var targetEntries: [noneEntry()]
  property int currentEntryIndex: -1

  readonly property var currentEntry: currentEntryIndex >= 0 ? targetEntries[currentEntryIndex] : null
  // Effective color of the destination track (its own color or the
  // track-type default); neutral while no destination is set
  readonly property color destinationColor: {
    const track = root.currentEntry?.track ?? null;
    return track !== null ? TrackColors.effectiveColor(track, ZrythmTheme.darkMode) : palette.mid;
  }
  readonly property bool sendEnabled: root.send.enabledParam.range.isToggled(root.send.enabledParam.baseValue)
  readonly property string amountValueText: {
    const realAmount = root.send.amountParam.range.convertFrom0To1(root.send.amountParam.baseValue);
    const dbfs = QmlUtils.amplitudeToDbfs(realAmount);
    if (dbfs < -98)
      return "-\u221e dB";
    return Number(dbfs).toFixed(dbfs < -10 ? 0 : 1) + " dB";
  }

  // A disabled send dims the whole row; an unconfigured send (no
  // destination yet) keeps full opacity so picking a destination still
  // reads as the enabled next step
  opacity: !root.send.hasDestination || root.sendEnabled ? 1.0 : ZrythmTheme.disabledOpacityFactor

  spacing: 4

  TrackOperator {
    id: sendOp

    channelSendRouting: root.channelSendRouting
    project: root.project
    track: root.track
    undoStack: root.undoStack
  }

  function noneEntry(): var {
    return {
      "key": "",
      "display": qsTr("None"),
      "track": null,
      "port": null
    };
  }

  function refreshTargets(): void {
    const entries = [root.noneEntry()];
    const targets = root.channelSendRouting.sendTargetsFor(root.track);
    for (const target of targets)
      entries.push(target);
    root.targetEntries = entries;
    root.syncCurrentIndex();
  }

  function syncCurrentIndex(): void {
    const key = root.channelSendRouting.sendDestinationKey(root.send);
    for (let i = 0; i < root.targetEntries.length; ++i) {
      if (root.targetEntries[i].key === key) {
        root.currentEntryIndex = i;
        return;
      }
    }
    root.currentEntryIndex = -1;
  }

  Component.onCompleted: root.refreshTargets()

  Connections {
    function onDestinationPortChanged() {
      root.refreshTargets();
    }

    target: root.send
  }

  // Enable dot: filled with the destination color when the send is
  // enabled, hollow when disabled
  Button {
    id: enableDot

    Accessible.name: qsTr("Send enabled")
    enabled: root.send.hasDestination
    flat: true
    implicitHeight: 18
    implicitWidth: 18
    padding: 0

    background: Rectangle {
      color: enableDot.hovered || enableDot.visualFocus || enableDot.down ? ZrythmTheme.buttonHoverBackgroundAppendColor : "transparent"
      radius: ZrythmTheme.textFieldRadius
    }

    contentItem: Item {
      Rectangle {
        anchors.centerIn: parent
        border.color: Qt.alpha(enableDot.palette.buttonText, 0.25)
        border.width: root.sendEnabled ? 0 : 1
        color: root.sendEnabled ? root.destinationColor : "transparent"
        height: 12
        radius: 6
        width: 12
      }
    }

    onClicked: {
      const param = root.send.enabledParam;
      param.setBaseValueByUser(param.range.isToggled(param.baseValue) ? 0.0 : 1.0);
    }

    ToolTip {
      text: qsTr("Send enabled")
    }
  }

  // Amount bar with the destination's color and name; click opens the
  // destination menu, right-click opens the same menu
  AmountBar {
    id: amountBar

    Layout.fillWidth: true

    amount: root.send.amountParam.baseValue
    fillColor: root.destinationColor
    label: root.currentEntry?.display ?? qsTr("None")
    valueText: root.amountValueText

    onAmountModified: newAmount => root.parameterOperator.setValue(root.send.amountParam, newAmount)
    onDragEnded: root.send.amountParam.endUserGesture()
    onDragStarted: root.send.amountParam.beginUserGesture()
    onTapped: destMenu.popup()

    ContextMenu.menu: Menu {
      id: destMenu

      onAboutToShow: root.refreshTargets()

      MenuItem {
        text: qsTr("Reset")

        onTriggered: root.parameterOperator.setValue(
          root.send.amountParam,
          root.send.amountParam.range.defaultNormalizedValue)
      }

      MenuSeparator {
      }

      Repeater {
        model: root.targetEntries

        delegate: MenuItem {
          required property var modelData

          text: modelData.display

          onTriggered: {
            // Unset entry fields are invalid QVariants, which QML reads
            // as undefined rather than null
            const targetPort = modelData.port ?? null;
            const targetTrack = modelData.track ?? null;
            if (targetPort !== null)
              sendOp.setSendTargetPort(root.send, targetPort);
            else if (targetTrack !== null)
              sendOp.setSendTargetTrack(root.send, targetTrack);
            else
              sendOp.setSendTargetTrack(root.send, null);
          }
        }
      }
    }

    ToolTip {
      text: qsTr("Send amount and destination")
    }
  }

  ChannelStateButton {
    Accessible.name: qsTr("Fader tap point")
    checkable: false
    checked: !root.send.isPrefader
    enabled: root.send.hasDestination
    ghost: true
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "fader.svg")

    onClicked: sendOp.setSendPrefader(root.send, !root.send.isPrefader)

    ToolTip {
      text: qsTr("Fader tap point (lit = post-fader)")
    }
  }

  Button {
    Accessible.name: qsTr("Remove this send")
    flat: true
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-cross.svg")
    icon.height: 12
    icon.width: 12
    padding: 2

    onClicked: sendOp.removeSend(root.send)

    ToolTip {
      text: qsTr("Remove this send")
    }
  }
}
