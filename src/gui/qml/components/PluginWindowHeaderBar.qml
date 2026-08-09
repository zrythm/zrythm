// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle

// Host chrome header strip shown above plugin editors. Instantiated from C++
// with a Plugin as initial property (rendered offscreen by the plugin host
// windows), or declared in QML; theme state comes from the shared application
// engine (QML singletons are per-engine). The root's implicitHeight drives
// the native header strip height.
// No ToolTips when rendered in the native strip: in-scene popups get clipped
// by the strip height.
Rectangle {
  id: root

  // Must be non-null: bindings dereference it unconditionally
  required property Plugin plugin

  readonly property color themeTextColor: ZrythmTheme.textColor
  readonly property color themeWindowColor: ZrythmTheme.pageColor
  // Margin around the controls row and name label
  readonly property int contentMargin: 4
  // Width reserved for the name label in the implicit width
  readonly property int minimumNameWidth: 40

  signal colorsChanged
  signal presetSelectorRequested

  color: themeWindowColor
  implicitHeight: controlsRow.implicitHeight + 2 * contentMargin
  implicitWidth: controlsRow.implicitWidth + 3 * contentMargin + minimumNameWidth

  onThemeTextColorChanged: colorsChanged()
  onThemeWindowColorChanged: colorsChanged()

  Row {
    id: controlsRow

    anchors.left: parent.left
    anchors.leftMargin: root.contentMargin
    anchors.verticalCenter: parent.verticalCenter
    spacing: 4

    ToolButton {
      id: bypassButton

      Accessible.name: qsTr("Bypass")
      checkable: true
      checked: root.plugin.bypassed
      flat: true
      focusPolicy: Qt.NoFocus
      font.family: ZrythmTheme.notoSansSymbols2Font.name
      palette.buttonText: root.themeTextColor
      text: "⏻"

      onClicked: {
        root.plugin.bypassed = !root.plugin.bypassed;
        // The control writes checked on click - restore the binding
        checked = Qt.binding(function () {
          return root.plugin.bypassed;
        });
      }
    }

    ToolButton {
      id: presetButton

      flat: true
      focusPolicy: Qt.NoFocus
      palette.buttonText: root.themeTextColor
      text: qsTr("Preset")

      onClicked: root.presetSelectorRequested()
    }

    ToolButton {
      id: abButton

      Accessible.name: qsTr("Compare two plugin states")
      display: AbstractButton.IconOnly
      flat: true
      focusPolicy: Qt.NoFocus
      icon.color: root.themeTextColor
      icon.source: root.plugin.abActive ? "qrc:/qt/qml/Zrythm/icons/zrythm-dark/preset-ba.svg" : "qrc:/qt/qml/Zrythm/icons/zrythm-dark/preset-ab.svg"
      palette.buttonText: root.themeTextColor
      text: qsTr("A/B")

      onClicked: root.plugin.switchAbState()
    }
  }

  Label {
    id: nameLabel

    anchors.left: controlsRow.right
    anchors.leftMargin: root.contentMargin
    anchors.right: parent.right
    anchors.rightMargin: root.contentMargin
    anchors.verticalCenter: parent.verticalCenter
    color: root.themeTextColor
    elide: Label.ElideRight
    // The configuration is only set once the plugin is instantiated
    text: root.plugin.configuration?.descriptor.name ?? ""
  }
}
