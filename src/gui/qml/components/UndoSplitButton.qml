// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

SplitButton {
  id: undoBtn

  required property bool isUndo
  required property var undoManager
  property var undoStack: isUndo ? undoManager.undoStack : undoManager.redoStack
  readonly property string undoString: isUndo ? qsTr("Undo") : qsTr("Redo")

  enabled: undoStack.rowCount > 0
  iconSource: ResourceManager.getIconUrl("zrythm-dark", "edit-" + (isUndo ? "undo" : "redo") + ".svg")
  menuTooltipText: qsTr("Undo multiple")
  tooltipText: undoStack.rowCount > 0 ? qsTr("%1 %2").arg(undoString).arg(undoStack.data(undoStack.index(0, 0), 257).getDescription()) : undoString

  menuItems: Menu {
    Repeater {
      model: undoStack

      delegate: MenuItem {
        required property var undoableAction

        text: qsTr("%1 %2").arg(undoString).arg(undoableAction.getDescription())

        onTriggered: {
          // TODO: implement properly
          isUndo ? undoManager.undo() : undoManager.redo();
        }
      }
    }
  }

  onClicked: isUndo ? undoManager.undo() : undoManager.redo()
}
