// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property AudioEngine audioEngine
  required property Channel channel
  property bool hovered: false
  required property OperationDispatcher operationDispatcher
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property PortObservationManager portObservationManager
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property Track track
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  // Returns a fresh source-model index for this delegate's track. Must be
  // called at time of use - QModelIndex values must not be cached in QML
  // because they are frozen snapshots that go stale when model rows change.
  function trackModelIndex(): var {
    return tracklist.collection.indexForTrack(track);
  }

  readonly property bool channelMuted: root.channel.fader.mute ? root.channel.fader.mute.baseValue > 0.5 : false
  // Whether the fader and meter dB legends fit the current fader height
  // (progressive disclosure: they disappear on short strips, where the
  // legend rows would collide)
  readonly property bool showFaderScale: faderControl.height >= 100

  // Shared mixer section state, owned by the mixer view: the fold state
  // of each level (folding folds it in every strip) and the uniform body
  // heights each section takes across strips
  property MixerSectionMetrics sectionMetrics: null

  // Whether this channel carries the MIDI FX level
  readonly property bool supportsMidiFx: !!root.channel.midiOut

  // This strip's own section content heights (0 where the level is not
  // supported); reported to sectionMetrics
  readonly property real midiFxContentHeight: root.supportsMidiFx ? midiFxList.contentHeight : 0
  readonly property real insertsContentHeight: insertsList.contentHeight
  readonly property real sendsContentHeight: root.track.type === Track.Master ? 0 : sendsContent.implicitHeight

  function reportSectionMetrics(): void {
    root.sectionMetrics?.updateStrip(
      root, root.supportsMidiFx, root.midiFxContentHeight,
      root.insertsContentHeight, root.sendsContentHeight);
  }

  onMidiFxContentHeightChanged: root.reportSectionMetrics()
  onInsertsContentHeightChanged: root.reportSectionMetrics()
  onSendsContentHeightChanged: root.reportSectionMetrics()
  onSupportsMidiFxChanged: root.reportSectionMetrics()
  Component.onCompleted: root.reportSectionMetrics()
  Component.onDestruction: root.sectionMetrics?.forgetStrip(root)

  implicitWidth: 112

  SelectionTracker {
    id: selectionTracker

    modelIndexProvider: root.trackModelIndex
    selectionModel: root.trackSelectionModel
  }

  Rectangle {
    Layout.fillHeight: true
    Layout.fillWidth: true
    color: ZrythmTheme.surfaceColor (
      root.palette.window, root.palette.windowText,
      selectionTracker.isSelected || bgTapHandler.pressed, root.hovered)

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }

    HoverHandler {
      onHoveredChanged: root.hovered = hovered
    }

    TapHandler {
      id: bgTapHandler

      acceptedButtons: Qt.LeftButton | Qt.RightButton
      acceptedModifiers: Qt.NoModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.selectSingleTrack(root.trackModelIndex());
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.select(root.trackModelIndex(), ItemSelectionModel.Toggle);
        root.trackSelectionModel.setCurrentIndex(root.trackModelIndex(), ItemSelectionModel.NoUpdate);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        const thisIdx = root.trackModelIndex();
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, thisIdx.row);
          const bottom = Math.max(currentIdx.row, thisIdx.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.ClearAndSelect);
        } else {
          root.trackSelectionModel.setCurrentIndex(thisIdx, ItemSelectionModel.Select);
        }
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        const thisIdx = root.trackModelIndex();
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, thisIdx.row);
          const bottom = Math.max(currentIdx.row, thisIdx.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.Select);
        } else {
          root.trackSelectionModel.setCurrentIndex(thisIdx, ItemSelectionModel.Select);
        }
      }
    }

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 4
      spacing: 2

      // Strip header: track color bar, type icon, name
      ColumnLayout {
        // Nested layouts only stretch past their implicit size with an
        // explicit maximum
        Layout.fillWidth: true
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        spacing: 2

        Rectangle {
          Layout.fillWidth: true
          Layout.preferredHeight: 4
          color: TrackColors.effectiveColor(root.track, ZrythmTheme.darkMode)
        }

        RowLayout {
          Layout.fillWidth: true
          Layout.leftMargin: 2
          Layout.rightMargin: 2
          spacing: 4

          IconImage {
            Layout.alignment: Qt.AlignVCenter
            color: nameLabel.color
            opacity: nameLabel.opacity
            source: TrackIcons.iconUrl(root.track)
            sourceSize.height: 14
            sourceSize.width: 14
          }

          Label {
            id: nameLabel

            Layout.fillWidth: true
            elide: Text.ElideRight
            font: selectionTracker.isSelected ? ZrythmTheme.buttonTextFont : ZrythmTheme.trackNameTextFont
            opacity: root.channelMuted ? ZrythmTheme.disabledOpacityFactor : 1
            text: root.track.name
          }
        }
      }

      // MIDI FX; channels without the level reserve its height as blank
      // space below so the following sections stay aligned
      ExpanderBox {
        id: midiFxSection

        Layout.fillWidth: true
        compact: true
        externalExpandedState: root.sectionMetrics !== null
        expanded: root.sectionMetrics ? root.sectionMetrics.midiFxExpanded : true
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "midi-insert.svg")
        title: qsTr("MIDI FX")
        visible: root.supportsMidiFx && (!root.sectionMetrics || root.sectionMetrics.anyMidiFx)

        onExpandedByUser: (willExpand) => {
          if (root.sectionMetrics)
            root.sectionMetrics.midiFxExpanded = willExpand;
        }

        frameContentItem: Item {
          implicitHeight: root.sectionMetrics ? root.sectionMetrics.midiFxBodyHeight : midiFxList.contentHeight

          PluginSlotList {
            id: midiFxList

            anchors.fill: parent
            interactive: false
            pluginGroup: root.channel.midiFx
            pluginImporter: root.pluginImporter
            pluginOperator: root.pluginOperator
            track: root.track
            trackSelectionModel: root.trackSelectionModel

            onPluginClicked: function (plugin: Plugin) {
              root.pluginClicked(plugin);
            }
          }
        }
      }

      Item {
        Layout.fillWidth: true
        Layout.preferredHeight: {
          if (!root.sectionMetrics || !root.sectionMetrics.anyMidiFx || root.supportsMidiFx)
            return 0;
          return (root.sectionMetrics.midiFxExpanded ? root.sectionMetrics.midiFxBodyHeight : 0) + midiFxSection.headerBlockHeight;
        }
        visible: root.sectionMetrics && root.sectionMetrics.anyMidiFx && !root.supportsMidiFx
      }

      // Inserts
      ExpanderBox {
        id: insertsSection

        Layout.fillWidth: true
        compact: true
        externalExpandedState: root.sectionMetrics !== null
        expanded: root.sectionMetrics ? root.sectionMetrics.insertsExpanded : true
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "audio-insert.svg")
        title: qsTr("Inserts")

        onExpandedByUser: (willExpand) => {
          if (root.sectionMetrics)
            root.sectionMetrics.insertsExpanded = willExpand;
        }

        frameContentItem: Item {
          implicitHeight: root.sectionMetrics ? root.sectionMetrics.insertsBodyHeight : insertsList.contentHeight

          PluginSlotList {
            id: insertsList

            anchors.fill: parent
            interactive: false
            pluginGroup: root.channel.inserts
            pluginImporter: root.pluginImporter
            pluginOperator: root.pluginOperator
            track: root.track
            trackSelectionModel: root.trackSelectionModel

            onPluginClicked: function (plugin: Plugin) {
              root.pluginClicked(plugin);
            }
          }
        }
      }

      // Sends; the master strip reserves the level's height as blank
      // space instead
      ExpanderBox {
        id: sendsSection

        Layout.fillWidth: true
        compact: true
        externalExpandedState: root.sectionMetrics !== null
        expanded: root.sectionMetrics ? root.sectionMetrics.sendsExpanded : true
        title: qsTr("Sends")
        visible: root.track.type !== Track.Master

        headerAction: Component {
          ChannelStateButton {
            Accessible.name: qsTr("Add send")
            checkable: false
            ghost: true
            icon.source: ResourceManager.getIconUrl("zrythm-dark", "add.svg")

            onClicked: addSendOp.addSend(false)

            ToolTip {
              text: qsTr("Add a post-fader send to this channel")
            }
          }
        }

        onExpandedByUser: (willExpand) => {
          if (root.sectionMetrics)
            root.sectionMetrics.sendsExpanded = willExpand;
        }

        frameContentItem: Item {
          implicitHeight: root.sectionMetrics ? root.sectionMetrics.sendsBodyHeight : sendsContent.implicitHeight

          ColumnLayout {
            id: sendsContent

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            spacing: 2

            TrackOperator {
              id: addSendOp

              track: root.track
              undoStack: root.undoStack
            }

            Repeater {
              model: root.channel.sends

              delegate: ChannelSendControls {
                Layout.fillWidth: true
                channelSendRouting: root.tracklist.channelSendRouting
                parameterOperator: root.parameterOperator
                project: root.project
                track: root.track
                undoStack: root.undoStack
              }
            }
          }
        }
      }

      Item {
        Layout.fillWidth: true
        Layout.preferredHeight: root.track.type === Track.Master && root.sectionMetrics ? (root.sectionMetrics.sendsExpanded ? root.sectionMetrics.sendsBodyHeight : 0) + sendsSection.headerBlockHeight : 0
        visible: root.track.type === Track.Master
      }

      // Stereo balance
      BalanceControl {
        Layout.fillWidth: true
        Layout.leftMargin: 2
        Layout.rightMargin: 2
        balanceParameter: root.channel.fader.balance
        parameterOperator: root.parameterOperator
      }

      // Fader with its dB legend, and the channel meters
      RowLayout {
        // Nested layouts only stretch past their implicit size with an
        // explicit maximum
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        Layout.minimumHeight: 80
        spacing: 4

        Item {
          Layout.fillWidth: true
        }

        FaderControl {
          id: faderControl

          Layout.alignment: Qt.AlignHCenter
          Layout.fillHeight: true
          faderGain: root.channel.fader.gain
          parameterOperator: root.parameterOperator
          showScale: root.showFaderScale
        }

        TrackMeters {
          id: trackMeters

          Layout.fillHeight: true
          audioEngine: root.audioEngine
          channel: root.channel
          portObservationManager: root.portObservationManager
          showScale: root.showFaderScale
          track: root.track
        }

        Item {
          Layout.fillWidth: true
        }
      }

      // Fader and peak level readouts
      FaderReadouts {
        // Nested layouts only stretch past their implicit size with an
        // explicit maximum
        Layout.fillWidth: true
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        faderText: faderControl.dbText
        peakOver: trackMeters.peakOver
        peakText: trackMeters.peakDbText
        peakVisible: trackMeters.hasAudio

        onResetPeak: trackMeters.resetPeakHold()
      }

      FaderButtons {
        // Nested layouts only stretch past their implicit size with an
        // explicit maximum
        Layout.fillWidth: true
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        fader: root.channel.fader
        track: root.track
      }

      TrackRouteControl {
        compact: true
        operationDispatcher: root.operationDispatcher
        track: root.track
        tracklist: root.tracklist
        undoStack: root.undoStack
      }
    }
  }
}
