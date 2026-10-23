// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property ChordPresetManager chordPresetManager
  readonly property Project project: session.project
  required property ProjectSession session
  required property TrackSelectionModel trackSelectionModel

  signal pluginClicked(Plugin plugin)

  implicitHeight: centerTabBar.implicitHeight + (mainStack.children[mainStack.currentIndex]?.implicitHeight ?? 0)
  spacing: 0

  StackLayout {
    id: mainStack

    Layout.fillHeight: true
    Layout.fillWidth: true
    currentIndex: centerTabBar.currentIndex

    StackLayout {
      id: editorContentAndNoRegionLabelStack

      Layout.fillHeight: true
      Layout.fillWidth: true
      currentIndex: root.session.uiState.clipEditor.clipObject ? 1 : 0

      PlaceholderPage {
        description: qsTr("Select a clip from the timeline")
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
        title: qsTr("No clip selected")
      }

      Loader {
        active: editorContentAndNoRegionLabelStack.currentIndex === 1
        visible: active

        sourceComponent: ClipEditorGrid {
          clipEditor: root.session.uiState.clipEditor
          session: root.session
        }
      }
    }

    MixerView {
      audioEngine: root.project.engine
      pluginImporter: root.session.pluginImporter
      pluginOperator: root.session.pluginOperator
      portObservationManager: root.project.portObservationManager
      trackSelectionModel: root.trackSelectionModel
      tracklist: root.project.tracklist
      undoStack: root.session.undoStack

      onPluginClicked: function(plugin: Plugin) {
        root.pluginClicked(plugin);
      }
    }

    ChordPadPanel {
      chordPresetManager: root.chordPresetManager
      session: root.session
    }
  }

  TabBar {
    id: centerTabBar

    Layout.fillWidth: true

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "piano-roll.svg")
      text: qsTr("Editor")
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "mixer.svg")
      text: qsTr("Mixer")
    }

    // TODO: re-enable when features are implemented
    // TabButton {
    //   icon.source: ResourceManager.getIconUrl("gnome-icon-library", "encoder-knob-symbolic.svg")
    //   text: qsTr("Modulators")
    // }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "chord-pad.svg")
      text: qsTr("Chord Pad")
    }
  }
}
