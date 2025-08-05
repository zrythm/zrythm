// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Item {
  id: root

  readonly property real contentHeight: listView.contentHeight + (pinned ? 0 : dropSpace.height)
  required property bool pinned
  required property var tracklist

  ListView {
    id: listView

    implicitHeight: 200
    implicitWidth: 200

    delegate: TrackView {
      width: ListView.view.width
    }
    model: TrackFilterProxyModel {
      sourceModel: root.tracklist

      Component.onCompleted: {
        addVisibilityFilter(true);
        addPinnedFilter(root.pinned);
      }
    }

    anchors {
      bottom: root.pinned ? parent.bottom : dropSpace.top
      left: parent.left
      right: parent.right
      top: parent.top
    }
  }

  Item {
    id: dropSpace

    height: 40
    visible: !root.pinned

    ContextMenu.menu: Menu {
      MenuItem {
        text: qsTr("Test")

        onTriggered: console.log("Clicked")
      }
    }

    anchors {
      bottom: parent.bottom
      left: parent.left
      right: parent.right
    }
  }
}
