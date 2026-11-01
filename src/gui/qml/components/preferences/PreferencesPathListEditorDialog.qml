// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Dialog {
  id: root

  signal pathsAccepted(var paths)

  property string pathKind: ""
  property var paths: []

  function addPath(path: string): bool {
    if (root.paths.includes(path))
      return false;
    root.paths = [...root.paths, path];
    return true;
  }

  function openWithPaths(paths: var): void {
    root.paths = [...paths];
    duplicatePathLabel.visible = false;
    root.open();
  }

  function removePath(index: int): void {
    root.paths = root.paths.filter(
      (path, pathIndex) => pathIndex !== index);
  }

  height: 380
  modal: true
  popupType: Popup.Window
  title: qsTr("Edit Paths")
  width: 520

  ColumnLayout {
    anchors.fill: parent
    spacing: ZrythmTheme.buttonPadding

    Label {
      Layout.fillWidth: true
      color: palette.text
      font: ZrythmTheme.smallTextFont
      opacity: 0.55
      text: qsTr("Leave the list empty to use the default locations.")
      wrapMode: Text.WordWrap
    }

    Button {
      text: qsTr("Add Folder…")

      onClicked: folderDialog.open()
    }

    ScrollView {
      Layout.fillHeight: true
      Layout.fillWidth: true
      clip: true
      contentWidth: availableWidth

      ListView {
        id: pathsList

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        model: root.paths
        spacing: ZrythmTheme.buttonPadding

        delegate: ActionRow {
          id: pathDelegate

          required property int index
          required property string modelData

          interactive: false
          title: pathDelegate.modelData
          width: pathsList.width

          DestructiveButton {
            text: qsTr("Remove")

            onClicked: root.removePath(pathDelegate.index)
          }
        }
      }
    }

    Label {
      id: duplicatePathLabel

      Layout.fillWidth: true
      color: ZrythmTheme.errorTextColor
      font: ZrythmTheme.smallTextFont
      text: qsTr("That folder is already in the list.")
      visible: false
    }
  }

  footer: DialogButtonBox {
    defaultButton: saveButton

    Button {
      DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
      text: qsTr("Cancel")
    }

    Button {
      id: saveButton

      DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
      highlighted: true
      text: qsTr("Save")
    }
  }

  FolderDialog {
    id: folderDialog

    onAccepted: {
      const path = QmlUtils.toPathString(folderDialog.selectedFolder);
      duplicatePathLabel.visible = !root.addPath(path);
    }
  }

  onAccepted: root.pathsAccepted(root.paths)
}
