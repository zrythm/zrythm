// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

Window {
  id: root

  // Nullable: assigned by the Instantiator after delegate construction.
  // The content components require a non-null plugin and are only
  // instantiated while one is set (see the Loader below)
  property Plugin plugin: null

  color: palette.window
  // Match the native plugin host windows: normal frame, kept above the
  // main window. The decoration hints must be spelled out: Windows only
  // expands bare Qt.Window into a decorated frame when no other hint is
  // set (no-ops on other platforms, which default to a decorated frame)
  flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
  height: 480
  minimumHeight: 160
  minimumWidth: 280
  title: root.plugin ? root.plugin.configuration.descriptor.name : ""
  width: 420

  onClosing: {
    // Defer so this window isn't destroyed mid-signal when the model row
    // removal reaches the Instantiator
    Qt.callLater(() => {
      if (root.plugin)
        root.plugin.uiVisible = false;
    });
  }

  Loader {
    active: root.plugin !== null
    anchors.fill: parent

    sourceComponent: ColumnLayout {
      spacing: 0

      PluginWindowHeaderBar {
        Layout.fillWidth: true
        plugin: root.plugin
      }

      PluginParameterListView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 8
        plugin: root.plugin
      }
    }
  }
}
