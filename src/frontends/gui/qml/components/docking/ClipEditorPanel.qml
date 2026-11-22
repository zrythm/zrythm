// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

StackLayout {
  id: root

  required property ProjectSession session

  currentIndex: root.session.uiState.clipEditor.clipObject ? 1 : 0
  implicitHeight: children[currentIndex]?.implicitHeight ?? 0

  PlaceholderPage {
    description: qsTr("Select a clip from the timeline")
    icon.source: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
    title: qsTr("No clip selected")
  }

  Loader {
    active: root.currentIndex === 1
    visible: active

    sourceComponent: ClipEditorGrid {
      clipEditor: root.session.uiState.clipEditor
      session: root.session
    }
  }
}
