// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle

// Host chrome header strip shown above embedded plugin editors. Instantiated
// from C++ with a PluginHostWindow as initial property. Rendered both in a
// visible QQuickWidget and offscreen for the X11 host window; theme state
// comes from the shared application engine (QML singletons are per-engine).
// No ToolTips: in-scene popups get clipped by the strip height.
Rectangle {
  id: root

  required property var hostWindow
  readonly property color themeWindowColor: ZrythmTheme.pageColor
  readonly property color themeTextColor: ZrythmTheme.textColor

  signal colorsChanged

  onThemeWindowColorChanged: colorsChanged()
  onThemeTextColorChanged: colorsChanged()

  color: themeWindowColor
  implicitWidth: controlsRow.implicitWidth + 52
  implicitHeight: 28

  Row {
    id: controlsRow

    anchors.left: parent.left
    anchors.leftMargin: 4
    anchors.verticalCenter: parent.verticalCenter
    spacing: 4

    ToolButton {
      id: bypassButton

      text: "⏻"
      font.family: ZrythmTheme.notoSansSymbols2Font.name
      checkable: true
      flat: true
      focusPolicy: Qt.NoFocus
      palette.buttonText: root.themeTextColor
      checked: root.hostWindow.bypassed
      Accessible.name: qsTr("Bypass")
      onClicked: {
        root.hostWindow.bypassToggleRequested();
        // The control writes checked on click - restore the binding
        checked = Qt.binding(function () {
          return root.hostWindow.bypassed;
        });
      }
    }

    ToolButton {
      id: presetButton

      text: qsTr("Preset")
      flat: true
      focusPolicy: Qt.NoFocus
      palette.buttonText: root.themeTextColor
      onClicked: root.hostWindow.presetSelectorRequested()
    }

    ToolButton {
      id: abButton

      flat: true
      focusPolicy: Qt.NoFocus
      text: qsTr("A/B")
      display: AbstractButton.IconOnly
      icon.source: root.hostWindow.abActive ? "qrc:/qt/qml/Zrythm/icons/zrythm-dark/preset-ba.svg" : "qrc:/qt/qml/Zrythm/icons/zrythm-dark/preset-ab.svg"
      icon.color: root.themeTextColor
      palette.buttonText: root.themeTextColor
      Accessible.name: qsTr("Compare two plugin states")
      onClicked: root.hostWindow.abSwitchRequested()
    }
  }

  Label {
    id: nameLabel

    anchors.left: controlsRow.right
    anchors.leftMargin: 4
    anchors.right: parent.right
    anchors.rightMargin: 4
    anchors.verticalCenter: parent.verticalCenter
    text: root.hostWindow.pluginName
    color: root.themeTextColor
    elide: Label.ElideRight
  }
}
