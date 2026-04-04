// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Control {
  id: root

  required property AudioEngine audioEngine
  readonly property real buttonHeight: 18
  readonly property real buttonPadding: 1
  readonly property real contentBottomMargins: 3
  readonly property real contentTopMargins: 1
  required property int depth
  readonly property real depthIndentation: depth * 8
  required property bool expanded
  required property bool foldable
  property bool isResizing: false
  property var listViewDraggedTrack: null
  property var listViewDropTargetFolder: null
  property int listViewDropTargetIndex: -1
  property bool listViewIsLast: false
  property int listViewPastEndIndex: -1
  required property Track track // connected automatically when used as a delegate for a Tracklist model
  required property int trackIndex
  readonly property var trackModelIndex: trackSelectionModel?.getModelIndex(trackIndex)
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  signal dropTargetChanged(int index)
  signal dropTargetFolderChanged(var track, int index)
  signal trackDragEnded
  signal trackDragStarted

  height: track.fullVisibleHeight
  hoverEnabled: true
  implicitHeight: 48
  implicitWidth: 200
  opacity: listViewDraggedTrack !== null && trackSelectionModel.isSelected(root.trackModelIndex) ? 0.5 : Style.getOpacity(track.enabled, root.Window.active)

  background: Rectangle {
    color: {
      let c = root.palette.window;
      if (root.hovered)
        c = Style.getColorBlendedTowardsContrast(c);

      if (selectionTracker.isSelected || bgTapHandler.pressed)
        c = Qt.alpha(root.palette.highlight, 0.15);

      return c;
    }

    Behavior on color {
      animation: Style.propertyAnimation
    }

    TapHandler {
      id: bgTapHandler

      acceptedButtons: Qt.LeftButton | Qt.RightButton
      acceptedModifiers: Qt.NoModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.selectSingleTrack(root.trackModelIndex);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier

      onTapped: (eventPoint, button) => {
        root.trackSelectionModel.select(root.trackModelIndex, ItemSelectionModel.Toggle);
        root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.NoUpdate);
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.trackModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.trackModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.ClearAndSelect);
        } else {
          root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.Select);
        }
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        const currentIdx = root.trackSelectionModel.currentIndex;
        if (currentIdx && currentIdx.valid) {
          const top = Math.min(currentIdx.row, root.trackModelIndex.row);
          const bottom = Math.max(currentIdx.row, root.trackModelIndex.row);
          const sel = QmlUtils.createRangeSelection(root.trackSelectionModel.model, top, bottom);
          root.trackSelectionModel.select(sel, ItemSelectionModel.Select);
        } else {
          root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.Select);
        }
      }
    }

    DragHandler {
      id: reorderDragHandler

      target: null
      xAxis.enabled: false

      onActiveChanged: {
        if (active) {
          // Select the dragged track if it wasn't already selected
          if (!root.trackSelectionModel.isSelected(root.trackModelIndex))
            root.trackSelectionModel.selectSingleTrack(root.trackModelIndex);
          root.trackDragStarted();
        } else {
          root.trackDragEnded();
        }
      }
      onTranslationChanged: {
        if (active) {
          const lv = root.ListView.view;
          if (!lv)
            return;

          const scenePos = centroid.scenePosition;
          // mapFromItem(null, ...) gives viewport-relative coordinates, but
          // itemAt() works in content coordinates. Add contentY to compensate
          // for scroll offset.
          const lvPoint = lv.mapFromItem(null, scenePos.x, scenePos.y);
          const contentPoint = Qt.point(lvPoint.x, lvPoint.y + lv.contentY);
          const targetItem = lv.itemAt(contentPoint.x, contentPoint.y) as TrackView;

          if (targetItem && targetItem !== root) {
            const targetLocalPoint = targetItem.mapFromItem(null, scenePos.x, scenePos.y);
            const relativeY = targetLocalPoint.y / targetItem.height;

            // Fixed-size edge zones (8px) for line-drop vs folder-drop
            const edgeZone = 8;
            if (targetItem.foldable && targetLocalPoint.y > edgeZone && targetLocalPoint.y < targetItem.height - edgeZone) {
              root.dropTargetFolderChanged(targetItem.track, targetItem.trackIndex + 1);
            } else {
              const idx = relativeY < 0.5 ? targetItem.trackIndex : targetItem.trackIndex + 1;
              root.dropTargetChanged(idx);
            }
          } else if (!targetItem && lvPoint.x >= 0 && lvPoint.x <= lv.width) {
            // Cursor is within the ListView horizontally but not over any
            // delegate - check if we're past the end of the track content.
            const footerH = lv.footerItem ? lv.footerItem.height : 0;
            const trackContentEnd = lv.contentHeight - footerH;
            const cursorContentY = lvPoint.y + lv.contentY;
            if (cursorContentY >= trackContentEnd - 10 && root.listViewPastEndIndex >= 0) {
              root.dropTargetChanged(root.listViewPastEndIndex);
            }
          }
        }
      }
    }
  }
  contentItem: Item {
    id: trackContent

    anchors.fill: parent

    RowLayout {
      id: mainLayout

      Layout.rightMargin: 6
      anchors.fill: parent
      spacing: 4

      Item {
        Layout.fillHeight: true
        Layout.preferredWidth: root.depthIndentation
      }

      Rectangle {
        id: trackColor

        readonly property int barWidth: 14
        readonly property int cornerRadius: 2

        Layout.fillHeight: true
        Layout.preferredWidth: barWidth
        color: root.track.color
        topLeftRadius: cornerRadius

        Button {
          id: expandButton

          height: trackColor.barWidth
          padding: 0
          visible: root.foldable && root.hovered
          width: trackColor.barWidth

          background: Rectangle {
            color: expandButton.hovered ? Qt.alpha(palette.button, 0.25) : Qt.alpha(palette.button, 0.1)
            radius: trackColor.cornerRadius
          }
          contentItem: IconImage {
            id: expandArrow

            readonly property int iconSize: trackColor.barWidth - 2

            color: palette.brightText
            height: iconSize
            rotation: root.expanded ? 0 : -90
            source: ResourceManager.getIconUrl("zrythm-dark", "adw-expander-arrow-symbolic.svg")
            sourceSize.height: iconSize
            sourceSize.width: iconSize
            width: iconSize

            Behavior on rotation {
              animation: Style.propertyAnimation
            }
          }

          onClicked: root.tracklist.collection.setTrackExpanded(root.track, !root.expanded)

          anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
          }
        }
      }

      ColumnLayout {
        id: allTrackComponentRows

        Layout.fillHeight: true
        Layout.fillWidth: true
        spacing: 0

        Loader {
          Layout.bottomMargin: 3
          Layout.fillWidth: true
          Layout.maximumHeight: root.track.height - Layout.bottomMargin - Layout.topMargin
          Layout.minimumHeight: root.track.height - Layout.bottomMargin - Layout.topMargin
          Layout.topMargin: 3
          sourceComponent: mainTrackView
        }

        Loader {
          id: lanesLoader

          Layout.fillHeight: true
          Layout.fillWidth: true
          active: root.track.lanes && root.track.lanes.lanesVisible
          visible: active

          sourceComponent: ColumnLayout {
            id: lanesColumnLayout

            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0

            ListView {
              id: lanesListView

              Layout.fillHeight: true
              Layout.fillWidth: true
              implicitHeight: contentHeight
              model: root.track.lanes

              delegate: ItemDelegate {
                id: laneDelegate

                required property TrackLane trackLane

                height: trackLane.height
                width: ListView.view.width

                RowLayout {
                  spacing: 2

                  anchors {
                    bottomMargin: root.contentBottomMargins
                    fill: parent
                    topMargin: root.contentTopMargins
                  }

                  Label {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillHeight: false
                    Layout.fillWidth: true
                    text: laneDelegate.trackLane.name
                  }

                  LinkedButtons {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.fillHeight: false
                    Layout.fillWidth: false

                    SoloButton {
                      id: laneSoloButton

                      padding: root.buttonPadding
                      styleHeight: root.buttonHeight
                      visible: false // currently unimplemented
                    }

                    MuteButton {
                      padding: root.buttonPadding
                      styleHeight: root.buttonHeight
                      visible: false // currently unimplemented
                    }
                  }
                }

                ResizeHandle {
                  resizeTarget: laneDelegate.trackLane

                  anchors.bottom: parent.bottom
                  anchors.left: parent.left
                  anchors.right: parent.right

                  onResizingChanged: (active) => {
                    root.isResizing = active;
                  }
                }

                Connections {
                  function onHeightChanged() {
                    root.track.fullVisibleHeightChanged();
                  }

                  target: laneDelegate.trackLane
                }
              }
            }
          }
        }

        Loader {
          id: automationLoader

          Layout.fillWidth: true
          active: root.track.automationTracklist && root.track.automationTracklist.automationVisible

          sourceComponent: AutomationTracksListView {
            id: automationTracksListView

            height: contentHeight
            track: root.track
            width: automationLoader.width
          }
        }
      }

      Loader {
        active: root.track.channel !== null
        visible: active

        sourceComponent: TrackMeters {
          Layout.fillHeight: true
          Layout.fillWidth: false
          audioEngine: root.audioEngine
          channel: root.track.channel
        }
      }
    }
  }
  Behavior on height {
    animation: Style.propertyAnimation
    // FIXME: stops working after some operations, not sure why
    enabled: !root.isResizing
  }

  SelectionTracker {
    id: selectionTracker

    modelIndex: root.trackModelIndex
    selectionModel: root.trackSelectionModel
  }

  Rectangle {
    id: dropIndicator

    color: palette.highlight
    height: 3
    visible: root.listViewDraggedTrack !== null && !root.trackSelectionModel.isSelected(root.trackModelIndex) && root.listViewDropTargetFolder === null && root.listViewDropTargetIndex === root.trackIndex && root.listViewDropTargetIndex >= 0
    z: 100

    anchors {
      left: parent.left
      right: parent.right
      top: parent.top
    }
  }

  Rectangle {
    id: dropIndicatorBottom

    color: palette.highlight
    height: 3
    visible: root.listViewDraggedTrack !== null && !root.trackSelectionModel.isSelected(root.trackModelIndex) && root.listViewDropTargetFolder === null && root.listViewIsLast && root.listViewDropTargetIndex === root.trackIndex + 1
    z: 100

    anchors {
      bottom: parent.bottom
      left: parent.left
      right: parent.right
    }
  }

  Rectangle {
    id: folderDropIndicator

    color: Qt.alpha(palette.highlight, 0.25)
    visible: root.listViewDraggedTrack !== null && root.foldable && root.listViewDropTargetFolder !== null && root.listViewDropTargetFolder === root.track
    z: 99

    anchors {
      bottom: parent.bottom
      bottomMargin: 1
      left: parent.left
      leftMargin: root.depthIndentation
      right: parent.right
      top: parent.top
      topMargin: 1
    }
  }

  Component {
    id: mainTrackView

    Item {
      id: mainTrackViewItem

      ColumnLayout {
        id: mainTrackViewColumnLayout

        spacing: 0

        anchors {
          bottomMargin: root.contentBottomMargins
          fill: parent
          topMargin: root.contentTopMargins
        }

        RowLayout {
          id: topRow

          readonly property real contentHeight: Math.max(trackNameLabel.implicitHeight, linkedButtons.implicitHeight)

          Layout.alignment: Qt.AlignTop
          Layout.fillHeight: true
          Layout.fillWidth: true
          Layout.minimumHeight: contentHeight
          Layout.preferredHeight: contentHeight

          IconImage {
            id: trackIcon

            readonly property real iconSize: 14

            Layout.alignment: Qt.AlignTop | Qt.AlignBaseline
            Layout.preferredHeight: iconSize
            Layout.preferredWidth: iconSize
            color: trackNameLabel.color
            fillMode: Image.PreserveAspectFit
            source: {
              if (root.track.icon.startsWith("gnome-icon-library-"))
                return ResourceManager.getIconUrl("gnome-icon-library", root.track.icon.substring(19) + ".svg");

              if (root.track.icon.startsWith("fluentui-"))
                return ResourceManager.getIconUrl("fluentui", root.track.icon.substring(9) + ".svg");

              if (root.track.icon.startsWith("lorc-"))
                return ResourceManager.getIconUrl("lorc", root.track.icon.substring(5) + ".svg");

              if (root.track.icon.startsWith("jam-icons-"))
                return ResourceManager.getIconUrl("jam-icons", root.track.icon.substring(10) + ".svg");

              return ResourceManager.getIconUrl("zrythm-dark", root.track.icon + ".svg");
            }
            sourceSize.height: iconSize
            sourceSize.width: iconSize
          }

          Label {
            id: trackNameLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            Layout.fillWidth: true
            elide: Text.ElideRight
            font: selectionTracker.isSelected ? Style.buttonTextFont : Style.normalTextFont
            text: root.track.name
          }

          LinkedButtons {
            id: linkedButtons

            Layout.alignment: Qt.AlignRight | Qt.AlignBaseline | Qt.AlignTop
            Layout.fillWidth: false
            layer.enabled: true

            layer.effect: DropShadowEffect {
            }

            MuteButton {
              id: muteButton

              checked: root.track.channel && root.track.channel.fader.mute.baseValue > 0.5
              padding: root.buttonPadding
              styleHeight: root.buttonHeight
              visible: root.track.channel !== null

              Binding {
                property: "baseValue"
                target: root.track.channel ? root.track.channel.fader.mute : null
                value: muteButton.checked ? 1.0 : 0.0
              }
            }

            SoloButton {
              id: trackSoloButton

              checked: root.track.channel && root.track.channel.fader.solo.baseValue > 0.5
              padding: root.buttonPadding
              styleHeight: root.buttonHeight
              visible: root.track.channel !== null

              Binding {
                property: "baseValue"
                target: root.track.channel ? root.track.channel.fader.solo : null
                value: trackSoloButton.checked ? 1.0 : 0.0
              }
            }

            RecordButton {
              Layout.preferredHeight: trackSoloButton.height
              checked: root.track.recordableTrackMixin && root.track.recordableTrackMixin.recording
              padding: root.buttonPadding
              styleHeight: root.buttonHeight
              visible: root.track.recordableTrackMixin !== null

              onClicked: {
                root.track.recordableTrackMixin.recording = !root.track.recordableTrackMixin.recording;
              }
            }
          }
        }

        RowLayout {
          id: bottomRow

          Layout.alignment: Qt.AlignBottom
          Layout.fillHeight: false
          Layout.fillWidth: true
          visible: root.track.height > root.buttonHeight + height + 12

          Label {
            id: chordScalesLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
            font: Style.smallTextFont
            text: qsTr("Scales")
            visible: root.track.type === Track.Chord
          }

          Item {
            id: filler

            Layout.fillHeight: true
            Layout.fillWidth: true
          }

          LinkedButtons {
            id: bottomRightButtons

            Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            Layout.fillHeight: true
            layer.enabled: true

            layer.effect: DropShadowEffect {
            }

            Button {
              checkable: true
              checked: root.track.lanes && root.track.lanes.lanesVisible
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "list-compact-symbolic.svg")
              padding: root.buttonPadding
              styleHeight: root.buttonHeight
              visible: root.track.lanes !== null

              onClicked: {
                root.track.lanes.lanesVisible = !root.track.lanes.lanesVisible;
              }

              ToolTip {
                text: qsTr("Show lanes")
              }
            }

            Button {
              checkable: true
              checked: root.track.automationTracklist && root.track.automationTracklist.automationVisible
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
              padding: root.buttonPadding
              styleHeight: root.buttonHeight
              visible: root.track.automationTracklist !== null

              onClicked: {
                root.track.automationTracklist.automationVisible = !root.track.automationTracklist.automationVisible;
              }

              ToolTip {
                text: qsTr("Show automation")
              }
            }
          }
        }
      }

      ResizeHandle {
        resizeTarget: root.track

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        onResizingChanged: (active) => {
          root.isResizing = active;
        }
      }
    }
  }

}
