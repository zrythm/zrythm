// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmDocking

ApplicationWindow {
  id: window

  property string savedLayout: ""

  function demoIcon(path: string): url {
    return Qt.resolvedUrl("../../../../src/gui/resources/icons/" + path);
  }

  height: 720
  title: qsTr("Zrythm Docking Demo")
  visible: true
  width: 1100

  header: ToolBar {
    RowLayout {
      anchors.fill: parent

      ToolButton {
        text: qsTr("Panels")

        onClicked: panelsMenu.popup()

        Menu {
          id: panelsMenu

          Instantiator {
            model: workspace.dockingController.panelIds

            delegate: MenuItem {
              required property string modelData
              readonly property DockPanel panel: workspace.dockingController.panelById(modelData)

              checkable: true
              checked: workspace.dockingController.layout.hidden.indexOf(modelData) < 0
              enabled: panel !== null
              icon.source: panel !== null ? panel.icon : ""
              text: panel !== null ? panel.title : modelData

              onTriggered: {
                if (checked)
                  workspace.dockingController.presentPanel(modelData);
                else
                  workspace.dockingController.hidePanel(modelData);
              }
            }

            onObjectAdded: (index, object) => panelsMenu.addItem(object)
            onObjectRemoved: (index, object) => panelsMenu.removeItem(object)
          }
        }
      }

      Button {
        text: qsTr("Reset Layout")

        onClicked: workspace.dockingController.resetLayout()
      }

      Button {
        text: qsTr("Save Layout")

        onClicked: window.savedLayout = workspace.dockingController.saveState()
      }

      Button {
        enabled: window.savedLayout.length > 0
        text: qsTr("Restore Saved")

        onClicked: workspace.dockingController.restoreState(window.savedLayout)
      }

      Label {
        Layout.fillWidth: true
        elide: Text.ElideRight
        text: qsTr("Drag tabs to reorder, split, or detach. Right-click tabs for more actions.")
      }
    }
  }

  DockWorkspace {
    id: workspace

    anchors.fill: parent
    defaultLayout: ({
      "version": 1,
      "main": {
        "areas": {
          "start": {
            "id": "left",
            "kind": "frame",
            "panels": ["inspector", "files"],
            "active": "inspector"
          },
          "end": {
            "id": "right",
            "kind": "frame",
            "panels": ["plugins", "monitor"],
            "active": "plugins"
          },
          "top": null,
          "bottom": {
            "id": "bottom",
            "kind": "frame",
            "panels": ["editor", "mixer"],
            "active": "editor",
            "tabsAtBottom": true
          },
          "center": {
            "id": "center",
            "kind": "frame",
            "panels": ["arrangement"],
            "active": "arrangement"
          }
        },
        "sizes": { "start": 240, "end": 260, "top": 200, "bottom": 220 }
      },
      "floating": [],
      "hidden": []
    })
    dockPalette: window.palette
    workspaceId: "docking-demo"

    DockPanel {
      defaultFrame: "left"
      icon: window.demoIcon("gnome-icon-library/general-properties-symbolic.svg")
      panelId: "inspector"
      title: qsTr("Inspector")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Inspector")
      }
    }

    DockPanel {
      defaultFrame: "left"
      icon: window.demoIcon("gnome-icon-library/file-cabinet-symbolic.svg")
      panelId: "files"
      title: qsTr("Files")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Files")
      }
    }

    DockPanel {
      defaultFrame: "right"
      icon: window.demoIcon("gnome-icon-library/shapes-large-symbolic.svg")
      panelId: "plugins"
      title: qsTr("Plugins")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Plugins")
      }
    }

    DockPanel {
      defaultFrame: "right"
      panelId: "monitor"
      title: qsTr("Monitor")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Monitor")
      }
    }

    DockPanel {
      defaultFrame: "bottom"
      panelId: "editor"
      title: qsTr("Editor")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Editor")
      }
    }

    DockPanel {
      defaultFrame: "bottom"
      panelId: "mixer"
      title: qsTr("Mixer")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Mixer")
      }
    }

    DockPanel {
      defaultFrame: "center"
      panelId: "arrangement"
      title: qsTr("Arrangement")

      MockPanelContent {
        anchors.fill: parent
        heading: qsTr("Arrangement")
      }
    }
  }
}
