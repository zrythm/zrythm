// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ZrythmStyle

ItemDelegate {
  id: root

  property bool alternate: false
  property bool interactive: false
  // Actions grouped under the row's "…" overflow button (list mode).
  property list<Action> overflowActions

  // Row hover/keyboard-focus state that reveals the list-mode action
  // buttons. Checked actions bypass this and stay visible at rest.
  readonly property bool rowActionsHot: root.interactive && (root.hovered || root.visualFocus || overflowMenu.opened)
  property string subtitle: ""
  // Shown as the subtitle when `subtitle` is empty (e.g. plugin format).
  property string subtitleFallback: ""

  // Suffix controls that are always visible (form mode: text fields,
  // combo boxes, switches next to the row's title/subtitle). Non-visual
  // children (actions, pointer handlers) belong in `resources:` instead,
  // so they attach to the row itself rather than the suffix area.
  default property alias suffix: suffixLayout.data
  // Action buttons revealed on row hover/keyboard focus (list mode).
  // Checked actions stay visible at rest (e.g. a favorited star).
  property list<Action> suffixActions
  property string title: ""

  Layout.fillWidth: true
  focusPolicy: root.interactive ? Qt.StrongFocus : Qt.NoFocus
  hoverEnabled: root.interactive
  icon.height: 16
  icon.width: 16
  implicitHeight: root.interactive ? 40 : root.implicitContentHeight + root.topPadding + root.bottomPadding

  background: Rectangle {
    color: {
      if (!root.interactive)
        return "transparent";
      if (root.highlighted)
        return root.palette.highlight;
      if (root.alternate)
        return ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(ZrythmTheme.alternateBackgroundColor, root.hovered, root.visualFocus, root.down);
      return (root.hovered || root.visualFocus || root.down) ? ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(root.palette.button, root.hovered, root.visualFocus, root.down) : "transparent";
    }
    radius: ZrythmTheme.textFieldRadius

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }
  contentItem: RowLayout {
    spacing: 8

    IconLabel {
      Layout.alignment: Qt.AlignVCenter
      display: IconLabel.IconOnly
      icon: root.icon
      visible: root.icon.source.toString() !== ""
    }

    ColumnLayout {
      Layout.alignment: Qt.AlignVCenter
      Layout.fillWidth: true
      spacing: 0

      Label {
        id: titleLabel

        Layout.fillWidth: true
        color: root.highlighted ? root.palette.highlightedText : root.palette.text
        elide: Text.ElideRight
        font: ZrythmTheme.semiBoldTextFont
        text: root.title
        visible: root.title !== ""
      }

      Label {
        id: subtitleLabel

        Layout.fillWidth: true
        color: root.highlighted ? root.palette.highlightedText : root.palette.text
        elide: Text.ElideRight
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        text: root.subtitle !== "" ? root.subtitle : root.subtitleFallback
        visible: text !== ""
      }
    }

    RowLayout {
      id: suffixLayout

      Layout.alignment: Qt.AlignVCenter
      spacing: 8
    }

    RowLayout {
      Layout.alignment: Qt.AlignVCenter

      Repeater {
        model: root.suffixActions

        delegate: ToolButton {
          id: suffixButton

          required property Action modelData

          Accessible.name: modelData.text
          action: modelData
          display: AbstractButton.IconOnly
          visible: modelData.checked || root.rowActionsHot

          // Checked actions keep their resting visibility via `checked` but
          // do not light up in the accent color; any active state is carried
          // by the icon itself (e.g. filled vs outline star)
          palette {
            buttonText: root.highlighted ? root.palette.highlightedText : root.palette.buttonText
            highlight: root.highlighted ? root.palette.highlightedText : root.palette.buttonText
          }

          ToolTip {
            text: suffixButton.modelData.text
          }
        }
      }
    }

    ToolButton {
      id: overflowButton

      text: "…"
      visible: root.overflowActions.length > 0 && root.rowActionsHot

      onClicked: overflowMenu.popup(overflowButton, 0, overflowButton.height)

      palette {
        buttonText: root.highlighted ? root.palette.highlightedText : root.palette.buttonText
        highlight: root.highlighted ? root.palette.highlightedText : root.palette.highlight
      }
    }
  }

  Menu {
    id: overflowMenu

    Instantiator {
      model: root.overflowActions

      delegate: MenuItem {
        required property Action modelData

        action: modelData
      }

      onObjectAdded: (index, object) => overflowMenu.insertItem(index, object)
      onObjectRemoved: (index, object) => overflowMenu.removeItem(object)
    }
  }
}
