// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

SplitButton {
  id: undoBtn

  readonly property var actions: undoBtn.isUndo ? undoBtn.undoStack?.undoActions : undoBtn.undoStack?.redoActions
  required property bool isUndo
  required property UndoStack undoStack
  readonly property string undoString: (isUndo ? qsTr("Undo") : qsTr("Redo"))

  enabled: (isUndo ? undoStack?.canUndo : undoStack?.canRedo) ?? false
  iconSource: ResourceManager.getIconUrl("zrythm-dark", "edit-" + (isUndo ? "undo" : "redo") + ".svg")
  menuTooltipText: isUndo ? qsTr("Undo multiple") : qsTr("Redo multiple")
  tooltipText: enabled ? "%1: %2".arg(undoString).arg(isUndo ? undoStack.undoActions[0] : undoStack.redoActions[0]) : undoString

  menuItems: Menu {
    id: menu

    Repeater {
      model: undoBtn.actions

      delegate: MenuItem {
        required property int index
        readonly property int indexToAdd: undoBtn.isUndo ? (-index - 1) : (index + 1)
        required property string modelData

        text: modelData

        onTriggered: {
          undoBtn.undoStack.index = undoBtn.undoStack.index + indexToAdd;
          menu.close();
        }
      }
    }
  }

  onClicked: isUndo ? undoStack.undo() : undoStack.redo()
}
