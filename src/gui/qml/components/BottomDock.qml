// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
  id: root

  required property Project project

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
      currentIndex: root.project.clipEditor.region ? 1 : 0

      PlaceholderPage {
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        Layout.fillHeight: true
        Layout.fillWidth: true
        description: qsTr("Select a clip from the timeline")
        icon.source: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
        title: qsTr("No clip selected")
      }

      Loader {
        active: editorContentAndNoRegionLabelStack.currentIndex === 1
        visible: active

        sourceComponent: ClipEditorGrid {
          clipEditor: root.project.clipEditor
          project: root.project
        }
      }
    }

    MixerView {
      tracklist: root.project.tracklist
      undoStack: root.project.undoStack
      pluginImporter: root.project.pluginImporter
    }

    Repeater {
      model: 2

      Rectangle {
        required property int index

        border.color: "black"
        border.width: 2
        color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
        height: 50
        width: 180

        Text {
          anchors.centerIn: parent
          font.pixelSize: 16
          text: "Item " + (parent.index + 1)
        }
      }
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

    TabButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "encoder-knob-symbolic.svg")
      text: qsTr("Modulators")
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "chord-pad.svg")
      text: qsTr("Chord Pad")
    }
  }
}
