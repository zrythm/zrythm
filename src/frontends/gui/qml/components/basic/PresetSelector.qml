// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Prev/next item selector with a popup browser.
 *
 * Shows the current item's name between previous and next buttons (which
 * wrap around at the edges); clicking the name opens a preset browser
 * popup. The control never changes currentIndex itself: user actions only
 * emit activated(), and the parent applies the selection (typically via a
 * binding on currentIndex), so external bindings stay intact.
 */
LinkedButtons {
  id: root

  // The popup's list also knows the count, but it can go stale while
  // the popup has never been shown, so prefer the model's own count
  readonly property int count: (root.model && root.model.count !== undefined) ? root.model.count : popupContent.count
  // Group of the current item, resolved by the parent from the model;
  // shown in the name button's tooltip
  property string currentGroup: ""
  // Index of the current item, or -1 when nothing is selected
  property int currentIndex: -1
  // Name of the current item, resolved by the parent from the model;
  // placeholderText is shown instead when empty
  property string currentText: ""
  // When true, the name button emits popupRequested() instead of opening
  // the in-scene popup; the host shows the list in a separate window (used
  // in native plugin strips, where an in-scene popup would be clipped by
  // the strip height)
  property bool externalPopup: false

  // Model providing the items; must be a QAbstractItemModel (e.g.
  // PluginPresetListModel or ListModel) — the in-scene popup connects to
  // its modelReset signal. The popup requires a "name" role for its
  // rows, and uses the model's `groups` and `plugin` properties when
  // present (the PluginPresetListModel surface). A model with a count
  // property (e.g. ListModel) gives the most up-to-date item count
  property var model: null
  readonly property alias nameButton: nameButton
  // The name button's tooltip
  readonly property alias nameToolTip: presetNameToolTip
  readonly property alias nextButton: nextButton
  // Text shown when there is no current item
  property string placeholderText: qsTr("No Preset")

  // Assigned to a property explicitly: the root's default property is
  // the linked button group, which only accepts items
  readonly property Popup popup: Popup {
    id: presetPopup

    background: null

    // Take focus so the browser can be navigated with the keyboard
    focus: true
    height: popupContent.implicitHeight
    padding: 0
    width: Math.max(nameButton.width, popupContent.implicitWidth)
    y: nameButton.height

    contentItem: PresetBrowserPopup {
      id: popupContent

      currentIndex: root.currentIndex
      model: root.model

      onActivated: idx => {
        root.activated(idx);
        presetPopup.close();
      }
      // Passive close (outside click, focus loss) commits the auditioned
      // selection; report it like an explicit activation so the parent
      // applies it (the popup is already closing)
      onApplied: idx => root.activated(idx)
      onCloseRequested: presetPopup.close()
    }
    enter: Transition {
      ParallelAnimation {
        PropertyAnimation {
          duration: ZrythmTheme.animationDuration
          easing.type: ZrythmTheme.animationEasingType
          from: nameButton.height / 2
          property: "y"
          to: nameButton.height
        }

        PropertyAnimation {
          duration: ZrythmTheme.animationDuration
          easing.type: ZrythmTheme.animationEasingType
          from: 0
          property: "opacity"
          to: 1
        }
      }
    }
    exit: Transition {
      PropertyAnimation {
        duration: ZrythmTheme.animationDuration
        easing.type: ZrythmTheme.animationEasingType
        property: "opacity"
        to: 0
      }
    }
  }
  // The popup's parent item; override (with popupType: Popup.Window) to
  // anchor the popup's window to a different window's scene
  property alias popupParent: presetPopup.parent
  // Popup.Item (in-scene) by default; set to Popup.Window to show the
  // popup in its own top-level window, e.g. over embedded plugin
  // windows that would otherwise cover or clip it
  property alias popupType: presetPopup.popupType
  readonly property bool popupVisible: popup.visible
  readonly property alias prevButton: prevButton

  // Emitted when the user picks an item via the arrows or the popup
  signal activated(int index)
  // Emitted instead of opening the popup when externalPopup is true
  signal popupRequested

  spacing: 0

  ToolButton {
    id: prevButton

    Accessible.name: qsTr("Previous Item")
    display: AbstractButton.IconOnly
    enabled: root.count > 0
    flat: true
    icon.source: ResourceManager.getIconUrl("noto-glyphs", "triangle-left.svg")

    // Wrap around at the edges; with nothing selected, wrap to the ends
    onClicked: root.activated(root.currentIndex < 0 ? root.count - 1 : (root.currentIndex - 1 + root.count) % root.count)
  }

  ToolButton {
    id: nameButton

    Layout.fillWidth: true
    Layout.minimumWidth: 120
    enabled: root.count > 0
    flat: true
    text: root.currentIndex >= 0 && root.currentText.length > 0 ? root.currentText : root.placeholderText

    contentItem: Label {
      color: nameButton.palette.buttonText
      elide: Text.ElideRight
      font: nameButton.font
      horizontalAlignment: Text.AlignHCenter
      text: nameButton.text
      verticalAlignment: Text.AlignVCenter
    }

    onClicked: {
      if (root.externalPopup)
        root.popupRequested();
      else
        root.popup.open();
    }

    ToolTip {
      id: presetNameToolTip

      text: (root.currentGroup.length > 0 ? root.currentGroup + " — " : "") + (root.currentIndex >= 0 ? root.currentText : "")
      // Nothing fits inside a strip-height native scene: a clamped
      // tooltip would cover the button and eat its clicks. The text
      // check avoids an empty bubble when nothing is selected
      visible: nameButton.hovered && !root.externalPopup && presetNameToolTip.text.length > 0
    }
  }

  ToolButton {
    id: nextButton

    Accessible.name: qsTr("Next Item")
    display: AbstractButton.IconOnly
    enabled: root.count > 0
    flat: true
    icon.source: ResourceManager.getIconUrl("noto-glyphs", "triangle-right.svg")

    // Wrap around at the edges; with nothing selected, wrap to the ends
    onClicked: root.activated(root.currentIndex < 0 ? 0 : (root.currentIndex + 1) % root.count)
  }
}
