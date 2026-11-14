// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Zrythm

Loader {
  id: root

  // Dense variant for mixer strips: hides the standalone unroute button
  // (unrouting stays available through the combo's context menu)
  property bool compact: false
  required property Track track
  required property Tracklist tracklist
  required property UndoStack undoStack

  Layout.fillWidth: true
  sourceComponent: root.track.type === Track.Master ? preroutedLabel : routingControls

  TrackOperator {
    id: trackOp

    track: root.track
    trackRouting: root.tracklist.trackRouting
    undoStack: root.undoStack
  }

  Component {
    id: preroutedLabel

    Label {
      color: palette.mid
      horizontalAlignment: Text.AlignHCenter
      text: qsTr("Prerouted")
      verticalAlignment: Text.AlignVCenter
    }
  }

  Component {
    id: routingControls

    RowLayout {
      spacing: 2

      ComboBox {
        id: routeTargetComboBox

        Layout.fillWidth: true
        textRole: "trackName"
        valueRole: "track"

        model: SortFilterProxyModel {
          id: availableRoutesModel

          sourceModel: root.tracklist.collection

          filters: [
            FunctionFilter {
              function filter(track: Track): bool {
                return root.tracklist.trackRouting.canRouteTo(root.track, track);
              }
            }
          ]
        }

        ContextMenu.menu: Menu {
          id: routeMenu

          MenuItem {
            enabled: routeTargetComboBox.currentIndex >= 0
            text: qsTr("Unroute")

            onTriggered: trackOp.setOutputTrack(null)
          }
        }

        // Right-click opens the menu directly, matching the track and
        // plugin-slot context menus
        TapHandler {
          acceptedButtons: Qt.RightButton

          onTapped: routeMenu.popup()
        }

        onActivated: trackOp.setOutputTrack(currentValue)

        Connections {
          function onRoutingChanged() {
            availableRoutesModel.invalidate();
            routeTargetComboBox.currentIndex = routeTargetComboBox.indexOfValue(root.tracklist.trackRouting.getOutputTrack(root.track));
          }

          target: root.tracklist.trackRouting
        }

        Component.onCompleted: {
          routeTargetComboBox.currentIndex = routeTargetComboBox.indexOfValue(root.tracklist.trackRouting.getOutputTrack(root.track));
        }
      }

      Button {
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-cross.svg")
        padding: 2
        visible: !root.compact && routeTargetComboBox.currentIndex >= 0

        onClicked: trackOp.setOutputTrack(null)

        ToolTip {
          text: qsTr("Unroute")
        }
      }
    }
  }
}
