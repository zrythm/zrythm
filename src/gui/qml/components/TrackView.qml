// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
  required property bool expanded
  required property bool foldable
  required property int trackIndex
  property bool isResizing: false
  required property Track track // connected automatically when used as a delegate for a Tracklist model
  readonly property var trackModelIndex: trackSelectionModel?.getModelIndex(trackIndex)
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  height: track.fullVisibleHeight
  hoverEnabled: true
  implicitHeight: 48
  implicitWidth: 200
  opacity: Style.getOpacity(track.enabled, root.Window.active)

  background: Rectangle {
    color: {
      let c = root.palette.window;
      if (root.hovered)
        c = Style.getColorBlendedTowardsContrast(c);

      if (selectionTracker.isSelected || bgTapHandler.pressed)
        c = Style.getColorBlendedTowardsContrast(c);

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
      }
    }

    TapHandler {
      acceptedModifiers: Qt.ShiftModifier

      onTapped: (eventPoint, button) => {
        if (root.trackSelectionModel.currentIndex.valid) {
          // Range selection (TODO)
          // const range = ItemSelectionRange(arrangerSelectionModel.currentIndex, unifiedIndex);
          // arrangerSelectionModel.select(range, ItemSelectionModel.SelectCurrent);
          console.log("Range selection unimplemented");
        } else {
          root.trackSelectionModel.setCurrentIndex(root.trackModelIndex, ItemSelectionModel.Select);
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
        Layout.preferredWidth: root.depth * 8
      }

      Rectangle {
        id: trackColor

        Layout.fillHeight: true
        Layout.preferredWidth: 6
        color: root.track.color

        Button {
          checkable: true
          checked: root.expanded
          text: root.expanded ? "−" : "+"
          visible: root.foldable

          onToggled: root.tracklist.collection.setTrackExpanded(root.track, !root.expanded)

          anchors {
            bottom: parent.bottom
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
                    }

                    MuteButton {
                      padding: root.buttonPadding
                      styleHeight: root.buttonHeight
                    }
                  }
                }

                Loader {
                  property var resizeTarget: laneDelegate.trackLane

                  anchors.bottom: parent.bottom
                  anchors.left: parent.left
                  anchors.right: parent.right
                  sourceComponent: resizeHandle
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

      Loader {
        property var resizeTarget: root.track

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        sourceComponent: resizeHandle
      }
    }
  }

  Component {
    id: resizeHandle

    Rectangle {
      id: resizeHandleRect

      property var resizeTarget: parent.resizeTarget

      color: "transparent"
      height: 6

      anchors {
        bottom: parent.bottom
        left: parent.left
        right: parent.right
      }

      Rectangle {
        anchors.centerIn: parent
        color: Qt.alpha(Style.backgroundAppendColor, 0.2)
        height: 2
        radius: 2
        width: 24
      }

      DragHandler {
        id: resizer

        property real startHeight: 0

        grabPermissions: PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlersOfSameType
        xAxis.enabled: false
        yAxis.enabled: true

        onActiveChanged: {
          root.isResizing = active;
          if (active)
            resizer.startHeight = resizeHandleRect.resizeTarget.height;
        }
        onTranslationChanged: {
          if (active) {
            let newHeight = Math.max(resizer.startHeight + translation.y, 24);
            resizeHandleRect.resizeTarget.height = newHeight;
          }
        }
      }

      MouseArea {
        acceptedButtons: Qt.NoButton // Pass through mouse events to the DragHandler
        anchors.fill: parent
        cursorShape: Qt.SizeVerCursor
      }
    }
  }
}
