// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
  id: control

  readonly property real buttonHeight: 18
  readonly property real buttonPadding: 1
  readonly property real contentBottomMargins: 3
  readonly property real contentTopMargins: 1
  property bool isResizing: false
  required property var track // connected automatically when used as a delegate for a Tracklist model

  hoverEnabled: true
  implicitHeight: track.fullVisibleHeight
  implicitWidth: 200
  opacity: Style.getOpacity(track.enabled, control.Window.active)

  background: Rectangle {
    color: {
      let c = control.palette.window;
      if (control.hovered)
        c = Style.getColorBlendedTowardsContrast(c);

      if (track.selected || control.down)
        c = Style.getColorBlendedTowardsContrast(c);

      return c;
    }

    Behavior on color {
      animation: Style.propertyAnimation
    }

    TapHandler {
      acceptedButtons: Qt.LeftButton | Qt.RightButton

      onTapped: function (point) {
        tracklist.setExclusivelySelectedTrack(control.track);
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

      Rectangle {
        id: trackColor

        Layout.fillHeight: true
        color: track.color
        width: 6
      }

      ColumnLayout {
        id: allTrackComponentRows

        Layout.fillHeight: true
        Layout.fillWidth: true
        spacing: 0

        Loader {
          Layout.bottomMargin: 3
          Layout.fillWidth: true
          Layout.maximumHeight: track.height - Layout.bottomMargin - Layout.topMargin
          Layout.minimumHeight: track.height - Layout.bottomMargin - Layout.topMargin
          Layout.topMargin: 3
          sourceComponent: mainTrackView
        }

        Loader {
          id: lanesLoader

          Layout.fillHeight: true
          Layout.fillWidth: true
          active: track.hasLanes && track.lanesVisible
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
              model: track.lanes

              delegate: ItemDelegate {
                readonly property var lane: modelData

                height: lane.height
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
                    text: lane.name
                  }

                  LinkedButtons {
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.fillHeight: false
                    Layout.fillWidth: false

                    SoloButton {
                      id: soloButton

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
                  property var resizeTarget: lane

                  anchors.bottom: parent.bottom
                  anchors.left: parent.left
                  anchors.right: parent.right
                  sourceComponent: resizeHandle
                }

                Connections {
                  function onHeightChanged() {
                    track.fullVisibleHeightChanged();
                  }

                  target: lane
                }
              }
            }
          }
        }

        Loader {
          id: automationLoader

          Layout.fillWidth: true
          active: track.isAutomatable && track.automatableTrackMixin.automationVisible

          sourceComponent: AutomationTracksListView {
            id: automationTracksListView

            height: contentHeight
            track: control.track
            width: automationLoader.width
          }
        }
      }

      Loader {
        id: audioMetersLoader

        Layout.fillHeight: true
        Layout.fillWidth: false
        active: track.hasChannel && track.channel.leftAudioOut
        visible: active

        sourceComponent: RowLayout {
          id: meters

          anchors.fill: parent
          spacing: 0

          Meter {
            Layout.fillHeight: true
            Layout.preferredWidth: width
            port: track.channel.leftAudioOut
          }

          Meter {
            Layout.fillHeight: true
            Layout.preferredWidth: width
            port: track.channel.rightAudioOut
          }
        }
      }

      Loader {
        id: midiMetersLoader

        Layout.fillHeight: true
        Layout.fillWidth: false
        Layout.preferredWidth: 4
        active: track.hasChannel && track.channel.midiOut
        visible: active

        sourceComponent: Meter {
          anchors.fill: parent
          port: track.channel.midiOut
        }
      }
    }
  }
  Behavior on implicitHeight {
    animation: Style.propertyAnimation
    // FIXME: stops working after some operations, not sure why
    enabled: !control.isResizing
  }

  Connections {
    function onAutomationVisibleChanged() {
      track.fullVisibleHeightChanged();
    }

    function onHeightChanged() {
      track.fullVisibleHeightChanged();
    }

    function onLanesVisibleChanged() {
      track.fullVisibleHeightChanged();
    }

    ignoreUnknownSignals: true
    target: track
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
            color: trackNameLabel.color
            fillMode: Image.PreserveAspectFit
            height: iconSize
            source: {
              if (track.icon.startsWith("gnome-icon-library-"))
                return ResourceManager.getIconUrl("gnome-icon-library", track.icon.substring(19) + ".svg");

              if (track.icon.startsWith("fluentui-"))
                return ResourceManager.getIconUrl("fluentui", track.icon.substring(9) + ".svg");

              if (track.icon.startsWith("lorc-"))
                return ResourceManager.getIconUrl("lorc", track.icon.substring(5) + ".svg");

              if (track.icon.startsWith("jam-icons-"))
                return ResourceManager.getIconUrl("jam-icons", track.icon.substring(10) + ".svg");

              return ResourceManager.getIconUrl("zrythm-dark", track.icon + ".svg");
            }
            sourceSize.height: iconSize
            sourceSize.width: iconSize
            width: iconSize
          }

          Label {
            id: trackNameLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
            Layout.fillWidth: true
            elide: Text.ElideRight
            font: track.selected ? Style.buttonTextFont : Style.normalTextFont
            text: track.name
          }

          LinkedButtons {
            id: linkedButtons

            Layout.alignment: Qt.AlignRight | Qt.AlignBaseline | Qt.AlignTop
            Layout.fillWidth: false
            layer.enabled: true

            layer.effect: DropShadowEffect {
            }

            MuteButton {
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
            }

            SoloButton {
              id: soloButton

              padding: control.buttonPadding
              styleHeight: control.buttonHeight
            }

            RecordButton {
              checked: track.isRecordable && track.recording
              height: soloButton.height
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: track.isRecordable

              onClicked: {
                track.recording = !track.recording;
              }
            }
          }
        }

        RowLayout {
          id: bottomRow

          Layout.alignment: Qt.AlignBottom
          Layout.fillHeight: false
          Layout.fillWidth: true
          visible: track.height > control.buttonHeight + height + 12

          Label {
            id: chordScalesLabel

            Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
            font: Style.smallTextFont
            text: qsTr("Scales")
            visible: track.type === Track.Chord
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
              checked: track.hasLanes && track.lanesVisible
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "list-compact-symbolic.svg")
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: track.hasLanes

              onClicked: {
                track.lanesVisible = !track.lanesVisible;
              }

              ToolTip {
                text: qsTr("Show lanes")
              }
            }

            Button {
              checkable: true
              checked: track.isAutomatable && track.automatableTrackMixin.automationVisible
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "automation-4p.svg")
              padding: control.buttonPadding
              styleHeight: control.buttonHeight
              visible: track.isAutomatable

              onClicked: {
                track.automatableTrackMixin.automationVisible = !track.automatableTrackMixin.automationVisible;
              }

              ToolTip {
                text: qsTr("Show automation")
              }
            }
          }
        }
      }

      Loader {
        property var resizeTarget: track

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

        grabPermissions: PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlers
        xAxis.enabled: false
        yAxis.enabled: true

        onActiveChanged: {
          control.isResizing = active;
          if (active)
            resizer.startHeight = resizeTarget.height;
        }
        onTranslationChanged: {
          if (active) {
            let newHeight = Math.max(resizer.startHeight + translation.y, 24);
            resizeTarget.height = newHeight;
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
