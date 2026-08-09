// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
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
  // main window
  flags: Qt.Window | Qt.WindowStaysOnTopHint
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
    anchors.fill: parent
    active: root.plugin !== null

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
