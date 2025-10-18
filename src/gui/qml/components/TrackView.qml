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
  id: control

  readonly property real buttonHeight: 18
  readonly property real buttonPadding: 1
  readonly property real contentBottomMargins: 3
  readonly property real contentTopMargins: 1
  required property int depth
  required property bool expanded
  required property bool foldable
  property bool isResizing: false
  required property Track track // connected automatically when used as a delegate for a Tracklist model
  property bool trackSelected: control.trackSelectionManager.isSelected(control.track)
  required property TrackSelectionManager trackSelectionManager
  required property Tracklist tracklist
  required property UndoStack undoStack

  height: track.fullVisibleHeight
  hoverEnabled: true
  implicitHeight: 48
  implicitWidth: 200
  opacity: Style.getOpacity(track.enabled, control.Window.active)

  background: Rectangle {
    color: {
      let c = control.palette.window;
      if (control.hovered)
        c = Style.getColorBlendedTowardsContrast(c);

      if (control.trackSelected || bgTapHandler.pressed)
        c = Style.getColorBlendedTowardsContrast(c);

      return c;
    }

    Behavior on color {
      animation: Style.propertyAnimation
    }

    TapHandler {
      id: bgTapHandler

      acceptedButtons: Qt.LeftButton | Qt.RightButton

      onTapped: function (point) {
        control.trackSelectionManager.selectUnique(control.track);
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
        Layout.preferredWidth: control.depth * 8
      }

      Rectangle {
        id: trackColor

        Layout.fillHeight: true
        Layout.preferredWidth: 6
        color: control.track.color

        Button {
          checkable: true
          checked: control.expanded
          text: control.expanded ? "−" : "+"
          visible: control.foldable

          onToggled: control.tracklist.collection.setTrackExpanded(control.track, !control.expanded)

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
          Layout.maximumHeight: control.track.height - Layout.bottomMargin - Layout.topMargin
          Layout.minimumHeight: control.track.height - Layout.bottomMargin - Layout.topMargin
          Layout.topMargin: 3
          sourceComponent: mainTrackView
        }

        Loader {
          id: lanesLoader

          Layout.fillHeight: true
          Layout.fillWidth: true
          active: control.track.lanes && control.track.lanes.lanesVisible
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
              model: control.track.lanes

              delegate: ItemDelegate {
                id: laneDelegate

                required property TrackLane trackLane

                height: trackLane.height
                width: ListView.view.width

                RowLayout {
                  spacing: 2

                  anchors {
                    bottomMargin: control.contentBottomMargins
                    fill: parent
                    topMargin: control.contentTopMargins
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

                      padding: control.buttonPadding
                      styleHeight: control.buttonHeight
                    }

                    MuteButton {
                      padding: control.buttonPadding
                      styleHeight: control.buttonHeight
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
                    control.track.fullVisibleHeightChanged();
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
          active: control.track.automationTracklist && control.track.automationTracklist.automationVisible

          sourceComponent: AutomationTracksListView {
            id: automationTracksListView

            height: contentHeight
            track: control.track
            width: automationLoader.width
          }
        }
      }

      Loader {
        active: control.track.channel !== null
        visible: active

        sourceComponent: TrackMeters {
          Layout.fillHeight: true
          Layout.fillWidth: false
          channel: control.track.channel
        }
      }
    }
  }
  Behavior on height {
    animation: Style.propertyAnimation
    // FIXME: stops working after some operations, not sure why
    enabled: !control.isResizing
  }

  Connections {
    function onSelectionChanged() {
      control.trackSelected = control.trackSelectionManager.isSelected(control.track);
    }

    target: control.trackSelectionManager
  }

  Component {
    id: mainTrackView

    Item {
      id: mainTrackViewItem

      ColumnLayout {
        id: mainTrackViewColumnLayout

        spacing: 0

        anchors {
          bottomMargin: control.contentBottomMargins
          fill: parent
          topMargin: control.contentTopMargins
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
              if (control.track.icon.startsWith("gnome-icon-library-"))
                return ResourceManager.getIconUrl("gnome-icon-library", control.track.icon.substring(19) + ".svg");

              if (control.track.icon.startsWith("fluentui-"))
                return ResourceManager.getIconUrl("fluentui", control.track.icon.substring(9) + ".svg");

              if (control.track.icon.startsWith("lorc-"))
                return ResourceManager.getIconUrl("lorc", control.track.icon.substring(5) + ".svg");

              if (control.track.icon.startsWith("jam-icons-"))
                return ResourceManager.getIconUrl("jam-icons", control.track.icon.substring(10) + ".svg");

              return ResourceManager.getIconUrl("zrythm-dark", control.track.icon + ".svg");
            }
            sourceSize.height: iconSize
            sourceSize.width: iconSize
          }

          Label {
            id: trackNameLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            Layout.fillWidth: true
            elide: Text.ElideRight
            font: control.trackSelected ? Style.buttonTextFont : Style.normalTextFont
            text: control.track.name
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

              checked: control.track.channel && control.track.channel.fader.mute.baseValue > 0.5
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: control.track.channel !== null

              Binding {
                property: "baseValue"
                target: control.track.channel ? control.track.channel.fader.mute : null
                value: muteButton.checked ? 1.0 : 0.0
              }
            }

            SoloButton {
              id: trackSoloButton

              checked: control.track.channel && control.track.channel.fader.solo.baseValue > 0.5
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: control.track.channel !== null

              Binding {
                property: "baseValue"
                target: control.track.channel ? control.track.channel.fader.solo : null
                value: trackSoloButton.checked ? 1.0 : 0.0
              }
            }

            RecordButton {
              Layout.preferredHeight: trackSoloButton.height
              checked: control.track.recordableTrackMixin && control.track.recordableTrackMixin.recording
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: control.track.recordableTrackMixin !== null

              onClicked: {
                control.track.recordableTrackMixin.recording = !control.track.recordableTrackMixin.recording;
              }
            }
          }
        }

        RowLayout {
          id: bottomRow

          Layout.alignment: Qt.AlignBottom
          Layout.fillHeight: false
          Layout.fillWidth: true
          visible: control.track.height > control.buttonHeight + height + 12

          Label {
            id: chordScalesLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
            font: Style.smallTextFont
            text: qsTr("Scales")
            visible: control.track.type === Track.Chord
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
              checked: control.track.lanes && control.track.lanes.lanesVisible
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "list-compact-symbolic.svg")
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: control.track.lanes !== null

              onClicked: {
                control.track.lanes.lanesVisible = !control.track.lanes.lanesVisible;
              }

              ToolTip {
                text: qsTr("Show lanes")
              }
            }

            Button {
              checkable: true
              checked: control.track.automationTracklist && control.track.automationTracklist.automationVisible
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: control.track.automationTracklist !== null

              onClicked: {
                control.track.automationTracklist.automationVisible = !control.track.automationTracklist.automationVisible;
              }

              ToolTip {
                text: qsTr("Show automation")
              }
            }
          }
        }
      }

      Loader {
        property var resizeTarget: control.track

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
          control.isResizing = active;
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
