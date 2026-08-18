// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle

/**
 * Item list with keyboard navigation, shown when a PresetSelector's name
 * button is clicked.
 *
 * Used as the content of PresetSelector's in-scene popup, and standalone
 * as the root of windowed popups over native plugin strips. Selection is
 * reported via activated(); the host applies it and closes the popup.
 */
Item {
  id: root

  // Index of the item to highlight, or -1 for none
  property int currentIndex: -1
  // Item count from the ListView; can go stale while never shown
  readonly property int count: listView.count
  // The list view, for hosts that need direct access (e.g. focus checks)
  readonly property alias listView: listView
  // Model providing the items (ListModel, QAbstractListModel, ...)
  property var model: null
  // When set (windowed hosts), activated()/closeRequested() are also
  // reported by calling the host's presetPopupActivated()/
  // presetPopupDismissed() invokables, since C++ cannot connect to this
  // component's QML-declared signals. Must outlive this item.
  property var popupHost: null
  // Role on the model that holds each item's name
  property string textRole: "display"

  // Emitted when the user picks an item (click, Enter/Return)
  signal activated(int index)
  // Emitted when the user dismisses the list without selecting (Escape)
  signal closeRequested

  onActivated: idx => {
    if (root.popupHost)
      root.popupHost.presetPopupActivated(idx);
  }
  onCloseRequested: {
    if (root.popupHost)
      root.popupHost.presetPopupDismissed();
  }

  implicitHeight: Math.min(listView.contentHeight, 360) + 4
  implicitWidth: 180
  // Windowed hosts (native plugin strips) don't inherit the main window's
  // palette. Bind each role instead of assigning the whole palette
  // object: whole-palette assignment copies the colors once and never
  // follows theme changes
  palette {
    accent: ZrythmTheme.colorPalette.accent
    alternateBase: ZrythmTheme.colorPalette.alternateBase
    base: ZrythmTheme.colorPalette.base
    brightText: ZrythmTheme.colorPalette.brightText
    button: ZrythmTheme.colorPalette.button
    buttonText: ZrythmTheme.colorPalette.buttonText
    dark: ZrythmTheme.colorPalette.dark
    highlight: ZrythmTheme.colorPalette.highlight
    highlightedText: ZrythmTheme.colorPalette.highlightedText
    light: ZrythmTheme.colorPalette.light
    link: ZrythmTheme.colorPalette.link
    linkVisited: ZrythmTheme.colorPalette.linkVisited
    mid: ZrythmTheme.colorPalette.mid
    midlight: ZrythmTheme.colorPalette.midlight
    placeholderText: ZrythmTheme.colorPalette.placeholderText
    shadow: ZrythmTheme.colorPalette.shadow
    text: ZrythmTheme.colorPalette.text
    toolTipBase: ZrythmTheme.colorPalette.toolTipBase
    toolTipText: ZrythmTheme.colorPalette.toolTipText
    window: ZrythmTheme.colorPalette.window
    windowText: ZrythmTheme.colorPalette.windowText
  }

  PopupBackgroundRect {
    anchors.fill: parent
  }

  ListView {
    id: listView

    anchors.fill: parent
    anchors.margins: 2
    clip: true
    currentIndex: root.currentIndex
    focus: true
    highlightMoveDuration: 0
    model: root.model

    ScrollIndicator.vertical: ScrollIndicator {
    }
    delegate: ItemDelegate {
      required property int index
      required property var model

      highlighted: listView.currentIndex === index
      text: model[root.textRole]
      width: listView.width

      onClicked: root.activated(index)
    }

    Keys.onEnterPressed: root.activated(listView.currentIndex)
    Keys.onEscapePressed: root.closeRequested()
    Keys.onReturnPressed: root.activated(listView.currentIndex)
  }

  // The list only becomes visible when its host shows it (popup open,
  // window shown); grab keyboard focus and scroll to the current item
  onVisibleChanged: {
    if (visible) {
      if (root.currentIndex >= 0 && listView.contentHeight > listView.height)
        listView.positionViewAtIndex(root.currentIndex, ListView.Center);
      listView.forceActiveFocus();
    }
  }
}
