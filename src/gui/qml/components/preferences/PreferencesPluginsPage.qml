// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import Qt.labs.synchronizer
import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  title: qsTr("Plugins")

  ActionRowGroup {
    title: qsTr("Behavior")
    subtitle: qsTr("What happens when plugins are added")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Open each plugin's editor as soon as it is added")
      title: qsTr("Open Plugin Editors On Instantiation")

      Switch {
        Synchronizer on checked {
          sourceObject: root.appSettings
          sourceProperty: "openPluginsOnInstantiation"
        }
      }
    }
  }

  ActionRowGroup {
    title: qsTr("Search Paths")
    subtitle: qsTr("Additional folders scanned the next time plugins are scanned")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Folders scanned for LV2 plugins")
      title: qsTr("LV2 Paths")

      Button {
        text: qsTr("Edit…")

        onClicked: {
          pathEditor.pathKind = "lv2";
          pathEditor.title = qsTr("Edit LV2 Paths");
          pathEditor.openWithPaths(root.appSettings?.lv2SearchPaths ?? []);
        }
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Folders scanned for VST3 plugins")
      title: qsTr("VST3 Paths")

      Button {
        text: qsTr("Edit…")

        onClicked: {
          pathEditor.pathKind = "vst3";
          pathEditor.title = qsTr("Edit VST3 Paths");
          pathEditor.openWithPaths(root.appSettings?.vst3SearchPaths ?? []);
        }
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Folders scanned for CLAP plugins")
      title: qsTr("CLAP Paths")

      Button {
        text: qsTr("Edit…")

        onClicked: {
          pathEditor.pathKind = "clap";
          pathEditor.title = qsTr("Edit CLAP Paths");
          pathEditor.openWithPaths(root.appSettings?.clapSearchPaths ?? []);
        }
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Folders scanned for Audio Unit plugins")
      title: qsTr("Audio Unit Paths")
      visible: Qt.platform.os === "osx"

      Button {
        text: qsTr("Edit…")

        onClicked: {
          pathEditor.pathKind = "au";
          pathEditor.title = qsTr("Edit Audio Unit Paths");
          pathEditor.openWithPaths(root.appSettings?.auSearchPaths ?? []);
        }
      }
    }
  }

  PreferencesPathListEditorDialog {
    id: pathEditor

    onPathsAccepted: (paths) => {
      if (root.appSettings === null)
        return;

      if (pathEditor.pathKind === "lv2")
        root.appSettings.lv2SearchPaths = paths;
      else if (pathEditor.pathKind === "vst3")
        root.appSettings.vst3SearchPaths = paths;
      else if (pathEditor.pathKind === "clap")
        root.appSettings.clapSearchPaths = paths;
      else if (pathEditor.pathKind === "au")
        root.appSettings.auSearchPaths = paths;
    }
  }
}
