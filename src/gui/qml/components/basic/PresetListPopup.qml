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
 *
 * When showGroupHeaders is enabled, items are separated by headers showing
 * their group (from the model's groupRole). Entries with an empty group
 * get no header.
 */
Item {
  id: root

  // Item count from the ListView; can go stale while never shown
  readonly property int count: listView.count

  // Index of the item to highlight, or -1 for none
  property int currentIndex: -1
  // Role on the model that holds each item's group
  property string groupRole: "group"
  // The list view, for hosts that need direct access (e.g. focus checks)
  readonly property alias listView: listView
  // Model providing the items; must be a QAbstractItemModel (e.g.
  // PluginPresetListModel or ListModel) — the modelReset signal is
  // required (plain JS arrays/ints are not supported)
  property var model: null
  // When set (windowed hosts), activated()/closeRequested() are also
  // reported by calling the host's presetPopupActivated()/
  // presetPopupDismissed() invokables, since C++ cannot connect to this
  // component's QML-declared signals. Must outlive this item.
  property var popupHost: null
  // Whether to show group section headers. Defaults to the model's
  // hasGroups property when it has one (e.g. PluginPresetListModel),
  // false otherwise
  property bool showGroupHeaders: root.model?.hasGroups ?? false
  // Role on the model that holds each item's name
  property string textRole: "display"

  // Emitted when the user picks an item (click, Enter/Return)
  signal activated(int index)
  // Emitted when the user dismisses the list without selecting (Escape)
  signal closeRequested

  implicitHeight: Math.min(listView.contentHeight, 360) + 4
  implicitWidth: 180

  onActivated: idx => {
    if (root.popupHost)
      root.popupHost.presetPopupActivated(idx);
  }
  onCloseRequested: {
    if (root.popupHost)
      root.popupHost.presetPopupDismissed();
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

    // Group headers are only meaningful when the list is ordered by group
    // (models are expected to emit entries grouped); section.property is
    // left unset when headers are disabled
    section.property: root.showGroupHeaders ? root.groupRole : ""

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
    section.delegate: Item {
      id: sectionDelegate

      required property string section

      // Entries without a group get no header
      height: section.length > 0 ? sectionLabel.implicitHeight + 10 : 0
      objectName: "groupHeader"
      visible: section.length > 0
      width: listView.width

      Label {
        id: sectionLabel

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        font.bold: true
        font.pixelSize: 10
        opacity: 0.7
        text: sectionDelegate.section
      }
    }

    Keys.onEnterPressed: root.activated(listView.currentIndex)
    Keys.onEscapePressed: root.closeRequested()
    Keys.onReturnPressed: root.activated(listView.currentIndex)

    // A model reset (e.g. preset list content changed) resets the view's
    // current index; re-apply the external selection. A plain assignment
    // would permanently remove the currentIndex binding above, so the
    // binding is re-established instead
    Connections {
      function onModelReset() {
        listView.currentIndex = Qt.binding(() => root.currentIndex);
      }

      target: root.model
    }
  }
}
