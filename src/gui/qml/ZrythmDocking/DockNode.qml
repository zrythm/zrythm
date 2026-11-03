// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Loader {
  id: root

  property string area: ""
  property DockingController controller: null
  property var node: null

  asynchronous: false
  sourceComponent: node === null ? (area.length > 0 ? emptyAreaComponent : null) : (node.kind === "split" ? splitComponent : frameComponent)

  Component {
    id: emptyAreaComponent

    EmptyDockArea {
      anchors.fill: parent
      area: root.area
      controller: root.controller
    }
  }

  Component {
    id: frameComponent

    DockFrame {
      anchors.fill: parent
      controller: root.controller
      node: root.node
    }
  }

  Component {
    id: splitComponent

    SplitView {
      id: splitView

      property bool resizeCompleted: false

      anchors.fill: parent
      implicitHeight: 120
      implicitWidth: 160
      orientation: root.node.orientation === "vertical" ? Qt.Vertical : Qt.Horizontal

      onResizingChanged: {
        if (resizing) {
          resizeCompleted = true;
        } else if (resizeCompleted) {
          resizeCompleted = false;
          const sizes = [];
          for (let i = 0; i < repeater.count; ++i) {
            const item = repeater.itemAt(i);
            sizes.push(orientation === Qt.Horizontal ? item.width : item.height);
          }
          root.controller.setSplitSizes(root.node.id, sizes);
        }
      }

      Repeater {
        id: repeater

        model: root.node.children

        delegate: Loader {
          id: childLoader

          required property int index
          required property var modelData

          SplitView.fillHeight: splitView.orientation === Qt.Vertical && index === repeater.count - 1
          SplitView.fillWidth: splitView.orientation === Qt.Horizontal && index === repeater.count - 1
          SplitView.minimumHeight: 80
          SplitView.minimumWidth: 100
          SplitView.preferredHeight: splitView.orientation === Qt.Vertical ? root.node.sizes[index] * splitView.height : implicitHeight
          SplitView.preferredWidth: splitView.orientation === Qt.Horizontal ? root.node.sizes[index] * splitView.width : implicitWidth
          source: "DockNode.qml"

          onLoaded: {
            const childNode = item as DockNode;
            childNode.controller = root.controller;
            childNode.node = childLoader.modelData;
          }
          onModelDataChanged: {
            if (status === Loader.Ready)
              (item as DockNode).node = childLoader.modelData;
          }
        }
      }
    }
  }
}
