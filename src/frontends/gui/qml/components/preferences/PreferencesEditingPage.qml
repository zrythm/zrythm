// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import Qt.labs.synchronizer
import QtQuick.Controls
import Zrythm
import ZrythmStyle

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  title: qsTr("Editing")

  ActionRowGroup {
    title: qsTr("General")
    subtitle: qsTr("Editing behavior shared by editors and the arranger")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Maximum number of undoable actions kept per project")
      title: qsTr("Undo Stack Length")

      SpinBox {
        from: 16
        stepSize: 16
        to: 1024

        Synchronizer on value {
          sourceObject: root.appSettings
          sourceProperty: "undoStackLength"
        }
      }
    }
  }
}
