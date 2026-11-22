// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmControllers
import ZrythmStyle

RowLayout {
  id: root

  required property AppSettings appSettings
  required property Metronome metronome
  property int playheadBar: 1
  property int playheadBeat: 1
  property real playheadBpm: 0
  property int playheadSixteenth: 1
  readonly property int playheadSixteenthsPerBeat: 16 / playheadTimeSigDenominator
  property int playheadTick: 0
  property int playheadTimeSigDenominator: 4
  property int playheadTimeSigNumerator: 4
  readonly property var sigDenominatorOptions: [2, 4, 8, 16]
  required property TempoMap tempoMap
  readonly property int ticksPerBar: ticksPerBeat * playheadTimeSigNumerator
  readonly property int ticksPerBeat: ticksPerSixteenth * playheadSixteenthsPerBeat
  readonly property int ticksPerSixteenth: tempoMap.getPpq() / 4
  required property Transport transport
  required property TransportController transportController
  required property UndoStack undoStack

  // Moves the playhead so that it differs from its current position by the
  // number of units (each unit being ticksPerUnit ticks) implied by newValue,
  // which is in absolute ticks. Used to scrub the position readout per part.
  function movePlayheadByUnits(newValue, ticksPerUnit) {
    const currentTicks = transport.playhead.ticks;
    const units = Math.round((newValue - currentTicks) / ticksPerUnit);
    if (units !== 0) {
      transport.movePlayhead(Math.max(0, currentTicks + units * ticksPerUnit), false);
    }
  }

  function refreshTempoMapReadouts() {
    // Round to the nearest tick: the playhead position is stored in samples,
    // so the tick value can carry a sub-tick error that makes floored values
    // flicker
    const ticks = Math.round(root.transport.playhead.ticks);
    playheadBpm = root.tempoMap.tempoAtTick(ticks);
    playheadBar = root.tempoMap.getMusicalPositionBar(ticks);
    playheadBeat = root.tempoMap.getMusicalPositionBeat(ticks);
    playheadSixteenth = root.tempoMap.getMusicalPositionSixteenth(ticks);
    playheadTick = root.tempoMap.getMusicalPositionTick(ticks);
    playheadTimeSigNumerator = root.tempoMap.timeSignatureNumeratorAtTick(ticks);
    playheadTimeSigDenominator = root.tempoMap.timeSignatureDenominatorAtTick(ticks);
  }

  Component.onCompleted: refreshTempoMapReadouts()

  Connections {
    function onBaseBpmChanged() {
      root.refreshTempoMapReadouts();
    }

    function onBaseTimeSignatureChanged() {
      root.refreshTempoMapReadouts();
    }

    function onTempoEventsChanged() {
      root.refreshTempoMapReadouts();
    }

    function onTimeSignatureEventsChanged() {
      root.refreshTempoMapReadouts();
    }

    target: root.tempoMap
  }

  Connections {
    function onTicksChanged() {
      root.refreshTempoMapReadouts();
    }

    target: root.transport.playhead
  }

  LinkedButtons {
    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-backward-large-symbolic.svg")

      onClicked: root.transportController.moveBackward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-forward-large-symbolic.svg")

      onClicked: root.transportController.moveForward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "skip-backward-large-symbolic.svg")
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", (root.transport.playState == 1 ? "pause" : "play") + "-large-symbolic.svg")

      onClicked: {
        root.transport.isRolling() ? root.transport.requestPause() : root.transport.requestRoll();
      }
    }

    Button {
      checkable: true
      checked: root.transport.loopEnabled
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")

      onCheckedChanged: {
        root.transport.loopEnabled = checked;
      }
    }
  }

  RecordSplitButton {
    appSettings: root.appSettings
    transport: root.transport
  }

  MetronomeSplitButton {
    id: metronomeSplitButton

    metronome: root.metronome
  }

  RowLayout {
    spacing: 2

    // BPM readout: follows the tempo at the playhead. Click to edit the base
    // tempo (the permanent tempo anchored at tick 0) in a dialog, or drag/scroll
    // to adjust it directly. The dot appears when the tempo at the playhead
    // differs from the base tempo.
    Control {
      id: bpmCell

      opacity: ZrythmTheme.getOpacity(bpmCell.enabled, bpmCell.Window.active)
      padding: 2

      contentItem: RowLayout {
        spacing: 3

        ScrubbableValueSegment {
          id: bpmSegment

          property bool valueChangedDuringDrag: false

          allowedAxes: Qt.Vertical
          from: 1
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 10000
          referenceText: "999.99"
          text: root.playheadBpm.toFixed(2)
          to: 999
          value: root.tempoMap.baseBpm
          wheelFineStep: 0.1 / (to - from)
          wheelStep: 1 / (to - from)

          onDragEnded: {
            if (!valueChangedDuringDrag) {
              bpmEditDialog.open();
            }
          }
          onDragStarted: {
            valueChangedDuringDrag = false;
          }
          onValueModified: function (newValue) {
            valueChangedDuringDrag = true;
            bpmPropertyOperator.setValueAffectingTempoMap("baseBpm", newValue);
          }
        }

        Text {
          color: bpmCell.palette.placeholderText
          font: ZrythmTheme.fadedTextFont
          text: "bpm"
        }

        Rectangle {
          id: baseTempoDiffDot

          Layout.alignment: Qt.AlignTop
          Layout.preferredHeight: 4
          Layout.preferredWidth: 4
          color: bpmCell.palette.accent
          radius: 3
          visible: Math.abs(root.playheadBpm - root.tempoMap.baseBpm) > 0.01
        }
      }

      ToolTip {
        text: baseTempoDiffDot.visible ? qsTr("Tempo at playhead differs from base tempo (%1 BPM)").arg(root.tempoMap.baseBpm.toFixed(2)) : qsTr("Base tempo: %1 BPM").arg(root.tempoMap.baseBpm.toFixed(2))
      }

      Dialog {
        id: bpmEditDialog

        modal: true
        popupType: Popup.Window
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Edit Base Tempo")

        contentItem: ColumnLayout {
          spacing: ZrythmTheme.buttonPadding

          Label {
            text: qsTr("Base BPM (at tick 0):")
          }

          DoubleSpinBox {
            id: bpmSpinBox

            Layout.fillWidth: true
            decimals: 2
            editable: true
            from: 1.0
            stepSize: 1.0
            to: 999.0
            value: root.tempoMap.baseBpm
          }
        }

        onAboutToShow: bpmSpinBox.value = root.tempoMap.baseBpm
        onAccepted: bpmPropertyOperator.setValueAffectingTempoMap("baseBpm", bpmSpinBox.value)
      }
    }

    // Position readout: follows the playhead. Drag or scroll on a part to move
    // the playhead by that unit (values wrap into neighboring parts). The
    // hovered part is highlighted.
    Control {
      id: positionCell

      opacity: ZrythmTheme.getOpacity(positionCell.enabled, positionCell.Window.active)
      padding: 2

      contentItem: RowLayout {
        spacing: 3

        ScrubbableValueSegment {
          id: barSegment

          readonly property real quantum: root.ticksPerBar

          allowedAxes: Qt.Vertical
          from: 0
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 15 * (to - from) / quantum
          referenceText: "99"
          text: root.playheadBar
          to: 1e9
          value: root.transport.playhead.ticks
          wheelFineStep: quantum / (to - from)
          wheelStep: quantum / (to - from)

          onValueModified: function (newValue) {
            root.movePlayheadByUnits(newValue, quantum);
          }
        }

        Text {
          color: positionCell.palette.text
          font: ZrythmTheme.semiBoldTextFont
          text: "."
        }

        ScrubbableValueSegment {
          id: beatSegment

          readonly property real quantum: root.ticksPerBeat

          allowedAxes: Qt.Vertical
          from: 0
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 15 * (to - from) / quantum
          referenceText: "99"
          text: root.playheadBeat
          to: 1e9
          value: root.transport.playhead.ticks
          wheelFineStep: quantum / (to - from)
          wheelStep: quantum / (to - from)

          onValueModified: function (newValue) {
            root.movePlayheadByUnits(newValue, quantum);
          }
        }

        Text {
          color: positionCell.palette.text
          font: ZrythmTheme.semiBoldTextFont
          text: "."
        }

        ScrubbableValueSegment {
          id: sixteenthSegment

          readonly property real quantum: root.ticksPerSixteenth

          allowedAxes: Qt.Vertical
          from: 0
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 15 * (to - from) / quantum
          referenceText: "99"
          text: root.playheadSixteenth
          to: 1e9
          value: root.transport.playhead.ticks
          wheelFineStep: quantum / (to - from)
          wheelStep: quantum / (to - from)

          onValueModified: function (newValue) {
            root.movePlayheadByUnits(newValue, quantum);
          }
        }

        Text {
          color: positionCell.palette.text
          font: ZrythmTheme.semiBoldTextFont
          text: "."
        }

        ScrubbableValueSegment {
          id: tickSegment

          readonly property real quantum: 1

          allowedAxes: Qt.Vertical
          from: 0
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 3 * (to - from) / quantum
          referenceText: "999"
          text: String(root.playheadTick).padStart(3, "0")
          to: 1e9
          value: root.transport.playhead.ticks
          wheelFineStep: quantum / (to - from)
          wheelStep: quantum / (to - from)

          onValueModified: function (newValue) {
            root.movePlayheadByUnits(newValue, quantum);
          }
        }

        Text {
          color: positionCell.palette.placeholderText
          font: ZrythmTheme.fadedTextFont
          text: "time"
        }
      }
    }

    // Time signature readout: follows the signature at the playhead. Drag or
    // scroll on either part to adjust the base signature (anchored at tick 0),
    // or click a part to edit both in a dialog. The hovered part is
    // highlighted.
    Control {
      id: sigCell

      opacity: ZrythmTheme.getOpacity(sigCell.enabled, sigCell.Window.active)
      padding: 2

      contentItem: RowLayout {
        spacing: 3

        ScrubbableValueSegment {
          id: sigNumeratorSegment

          property bool valueChangedDuringDrag: false

          allowedAxes: Qt.Vertical
          from: 1
          hoverCursorShape: Qt.SizeVerCursor
          referenceText: "99"
          text: root.playheadTimeSigNumerator
          to: 16
          value: root.tempoMap.baseTimeSignatureNumerator
          wheelFineStep: 1 / (to - from)
          wheelStep: 1 / (to - from)

          onDragEnded: {
            if (!valueChangedDuringDrag) {
              timeSigEditDialog.open();
            }
          }
          onDragStarted: {
            valueChangedDuringDrag = false;
          }
          onValueModified: function (newValue) {
            const numerator = Math.round(newValue);
            if (numerator !== root.tempoMap.baseTimeSignatureNumerator) {
              valueChangedDuringDrag = true;
              bpmPropertyOperator.setValueAffectingTempoMap("baseTimeSignatureNumerator", numerator);
            }
          }
        }

        Text {
          color: sigCell.palette.text
          font: ZrythmTheme.semiBoldTextFont
          text: "/"
        }

        ScrubbableValueSegment {
          id: sigDenominatorSegment

          property bool valueChangedDuringDrag: false

          allowedAxes: Qt.Vertical
          from: 0
          hoverCursorShape: Qt.SizeVerCursor
          pixelsForFullRange: 120
          referenceText: "99"
          text: root.playheadTimeSigDenominator
          to: root.sigDenominatorOptions.length - 1
          value: Math.max(0, root.sigDenominatorOptions.indexOf(root.tempoMap.baseTimeSignatureDenominator))
          wheelFineStep: 1 / (to - from)
          wheelStep: 1 / (to - from)

          onDragEnded: {
            if (!valueChangedDuringDrag) {
              timeSigEditDialog.open();
            }
          }
          onDragStarted: {
            valueChangedDuringDrag = false;
          }
          onValueModified: function (newValue) {
            const denominator = root.sigDenominatorOptions[Math.round(newValue)];
            if (denominator !== undefined && denominator !== root.tempoMap.baseTimeSignatureDenominator) {
              valueChangedDuringDrag = true;
              bpmPropertyOperator.setValueAffectingTempoMap("baseTimeSignatureDenominator", denominator);
            }
          }
        }

        Text {
          color: sigCell.palette.placeholderText
          font: ZrythmTheme.fadedTextFont
          text: "sig"
        }
      }

      Dialog {
        id: timeSigEditDialog

        modal: true
        popupType: Popup.Window
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Edit Time Signature")

        contentItem: ColumnLayout {
          spacing: ZrythmTheme.buttonPadding

          Label {
            text: qsTr("Beats per bar:")
          }

          SpinBox {
            id: sigNumeratorSpinBox

            Layout.fillWidth: true
            editable: true
            from: 1
            to: 16
            value: root.tempoMap.baseTimeSignatureNumerator
          }

          Label {
            text: qsTr("Beat unit:")
          }

          ComboBox {
            id: sigDenominatorComboBox

            Layout.fillWidth: true
            model: root.sigDenominatorOptions
          }
        }

        onAboutToShow: {
          sigNumeratorSpinBox.value = root.tempoMap.baseTimeSignatureNumerator;
          const idx = sigDenominatorComboBox.model.indexOf(root.tempoMap.baseTimeSignatureDenominator);
          sigDenominatorComboBox.currentIndex = idx >= 0 ? idx : 1;
        }
        onAccepted: {
          bpmPropertyOperator.setValueAffectingTempoMap("baseTimeSignatureNumerator", sigNumeratorSpinBox.value);
          bpmPropertyOperator.setValueAffectingTempoMap("baseTimeSignatureDenominator", sigDenominatorComboBox.currentValue);
        }
      }
    }
  }

  QObjectPropertyOperator {
    id: bpmPropertyOperator

    currentObject: root.tempoMap
    undoStack: root.undoStack
  }
}
