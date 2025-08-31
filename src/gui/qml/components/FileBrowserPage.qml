// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property FileSystemModel fileSystemModel

  signal fileActivated(string filePath)

  Item {
    id: filters

  }

  Item {
    id: searchBar

  }

  TreeView {
    id: fileListView

    property var selectedFile
    readonly property var selectionModel: ItemSelectionModel {
      model: fileListView.model
    }

    Layout.fillHeight: true
    Layout.fillWidth: true
    activeFocusOnTab: true
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    focus: true
    model: root.fileSystemModel
    rootIndex: root.fileSystemModel.rootIndex

    ScrollBar.vertical: ScrollBar {
    }
    delegate: TreeViewDelegate {
      id: itemDelegate

      required property int column
      required property var fileInfo
      required property string fileName
      required property string filePath
      required property int index

      Drag.active: false
      Drag.dragType: Drag.Automatic
      Drag.keys: ["file", "text/plain"]
      Drag.supportedActions: Qt.CopyAction
      highlighted: row == treeView.currentRow
      text: fileName + fileName
      width: fileListView.width

      action: Action {
        text: "Activate"

        onTriggered: {
          console.log("activated", itemDelegate.filePath);
          if (root.fileSystemModel.isDir(itemDelegate.fileInfo)) {} else {
            root.fileActivated(itemDelegate.filePath);
          }
        }
      }

      MouseArea {
        id: mouseArea

        anchors.fill: parent

        // This avoids the delegate onClicked triggering the action
        onClicked: {
          console.log('clicked');
          fileListView.selectedFile = itemDelegate.filePath;
        }
        onDoubleClicked: itemDelegate.animateClick()

        drag {
          smoothed: true
          target: itemDelegate

          // Set the data to be transferred during drag
          onActiveChanged: {
            if (drag.active) {
              fileListView.selectedFile = itemDelegate.filePath;
              parent.Drag.active = drag.active;
              parent.Drag.mimeData = {
                "text/plain": itemDelegate.filePath,
                "application/x-file-path": itemDelegate.filePath
              };
              parent.Drag.hotSpot = Qt.point(width / 2, height / 2);
            } else {
              parent.Drag.active = false;
            }
          }
        }
      }
    }
  }

  Label {
    id: pluginInfoLabel

    Layout.fillWidth: true
    text: root.fileSystemModel.getFileInfoAsString(fileListView.selectedFile)
  }
}
