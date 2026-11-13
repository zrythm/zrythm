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

  // Shared drag source for dragging files out of the browser: owned by the
  // page so it survives the destruction of the file delegate being dragged
  AutoDragItem {
    id: fileDragItem

    property string filePath: ""

    Drag.keys: ["file", "text/plain"]
    Drag.supportedActions: Qt.CopyAction
    Drag.mimeData: fileDragItem.filePath !== "" ? {
      "text/plain": fileDragItem.filePath,
      "application/x-file-path": fileDragItem.filePath
    } : ({})
  }

  Item {
    id: filters

  }

  Item {
    id: searchBar

  }

  TreeView {
    id: fileListView

    property var selectedFile

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

          onActiveChanged: {
            if (drag.active) {
              fileListView.selectedFile = itemDelegate.filePath;
              fileDragItem.filePath = itemDelegate.filePath;
              fileDragItem.Drag.active = true;
            } else {
              fileDragItem.Drag.active = false;
            }
          }
        }
      }
    }
    selectionModel: ItemSelectionModel {
      model: fileListView.model
    }
  }

  Label {
    id: pluginInfoLabel

    Layout.fillWidth: true
    text: root.fileSystemModel.getFileInfoAsString(fileListView.selectedFile)
  }
}
