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

          model: root.tracklist.collection

          filters: [
            FunctionFilter {
              function filter(data: TrackRoleData): bool {
                return root.tracklist.trackRouting.canRouteTo(root.track, data.track);
              }
            }
          ]
        }

        onActivated: trackOp.setOutputTrack(currentValue)

        Connections {
          function onRoutingChanged() {
            availableRoutesModel.invalidate();
            routeTargetComboBox.currentIndex = routeTargetComboBox.indexOfValue(root.tracklist.trackRouting.getOutputTrack(root.track));
          }

          target: root.tracklist.trackRouting
        }
      }

      Button {
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "draw-cross.svg")
        padding: 2
        visible: routeTargetComboBox.currentIndex >= 0

        onClicked: trackOp.setOutputTrack(null)

        ToolTip {
          text: qsTr("Unroute")
        }
      }
    }
  }

  component TrackRoleData: QtObject {
    property Track track
  }
}
