// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle

/**
 * Item list with keyboard navigation, used as the list pane of
 * PresetBrowserPopup and standalone where a plain list popup is enough.
 *
 * Standalone use reports the selection via activated(); the host applies
 * it and closes the popup. In composite use (clickActivates false),
 * clicks only move the highlight and double-clicks emit activated().
 *
 * When showGroupHeaders is enabled, items are separated by headers showing
 * their group (from the model's groupRole). Entries with an empty group
 * get no header.
 */
Item {
  id: root

  // Whether clicking an item emits activated() (standalone use). When
  // false (composite hosts), clicks only move the highlight;
  // double-clicks still emit activated()
  property bool clickActivates: true

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
  // Whether the list takes keyboard focus (statically and when becoming
  // visible); composite hosts that keep focus on another input (e.g. a
  // search field) set this to false
  property bool ownsKeyboardFocus: true
  // Whether the popup background is painted; composite hosts (e.g.
  // PresetBrowserPopup) paint their own
  property bool showBackground: true
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

  // The list only becomes visible when its host shows it (popup open,
  // window shown); grab keyboard focus and scroll to the current item
  onVisibleChanged: {
    if (visible) {
      if (root.currentIndex >= 0 && listView.contentHeight > listView.height)
        listView.positionViewAtIndex(root.currentIndex, ListView.Center);
      if (root.ownsKeyboardFocus)
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
    visible: root.showBackground
  }

  ListView {
    id: listView

    anchors.fill: parent
    anchors.margins: 2
    clip: true
    currentIndex: root.currentIndex
    focus: root.ownsKeyboardFocus
    highlightMoveDuration: 0
    model: root.model

    // Group headers are only meaningful when the list is ordered by group
    // (models are expected to emit entries grouped); section.property is
    // left unset when headers are disabled
    section.property: root.showGroupHeaders ? root.groupRole : ""

    // Interactive scrollbar; stays transient but is revealed on hover so
    // it can be dragged
    ScrollBar.vertical: ScrollBar {
      active: listView.moving || hovered || pressed
      hoverEnabled: true
      policy: ScrollBar.AsNeeded
    }
    delegate: ItemDelegate {
      id: presetDelegate

      required property int index
      required property var model

      // Clicks must not steal keyboard focus from composite hosts (e.g.
      // the browser's search field), or arrow-key navigation dies
      focusPolicy: Qt.NoFocus
      highlighted: listView.currentIndex === index
      text: model[root.textRole]
      width: listView.width

      onClicked: {
        if (root.clickActivates)
          root.activated(presetDelegate.index);
        else
          listView.currentIndex = presetDelegate.index;
      }

      // In browser mode (single click only selects), double-click applies
      // directly; the second click of the pair does not emit clicked()
      onDoubleClicked: {
        if (!root.clickActivates)
          root.activated(presetDelegate.index);
      }
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
        font: ZrythmTheme.semiBoldTextFont
        opacity: 0.8
        text: sectionDelegate.section
      }
    }

    Keys.onEnterPressed: root.activated(listView.currentIndex)
    Keys.onEscapePressed: root.closeRequested()
    Keys.onReturnPressed: root.activated(listView.currentIndex)

    // Internal navigation (clicks, keyboard) assigns currentIndex
    // directly, which removes the binding above; external updates are
    // therefore applied via this signal handler instead of relying on the
    // binding staying intact
    Connections {
      function onCurrentIndexChanged() {
        listView.currentIndex = root.currentIndex;
      }

      target: root
    }

    // A model reset (e.g. preset list content changed) resets the view's
    // current index; re-apply the external selection
    Connections {
      function onModelReset() {
        listView.currentIndex = root.currentIndex;
      }

      target: root.model
    }
  }
}
