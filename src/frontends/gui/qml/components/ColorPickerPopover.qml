// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

/**
 * Color picker popover: a default cell (marked with a slash), a row of
 * swatches, and a custom color cell that opens the system color dialog.
 *
 * The component is stateless: the caller supplies the current color
 * state and the color the default cell previews, and handles the
 * colorSelected()/unsetRequested() signals.
 */
Popup {
  id: root

  /** Currently set color; selected among the swatches when useColor is true. */
  property color currentColor: "transparent"

  /** Whether a color is currently set; the default cell is selected when false. */
  property bool useColor: false

  /** The color used when nothing is set, shown in the default cell. */
  property color defaultColor: "transparent"

  /** Swatch colors; defaults to the canonical track-type palette in the current theme mode. */
  readonly property var swatches: {
    const colors = [];
    for (let i = 0; i < TrackColors.canonicalTypes().length; i++) {
      colors.push(
        TrackColors.defaultColor(
          TrackColors.canonicalTypes()[i], ZrythmTheme.darkMode));
    }
    return colors;
  }

  signal colorSelected(color picked)
  signal unsetRequested()

  focus: true
  padding: 8

  /** True when the current color matches one of the swatches. */
  function usesSwatchColor(): bool {
    if (!root.useColor)
      return false;
    for (let i = 0; i < root.swatches.length; i++) {
      if (Qt.colorEqual(root.currentColor, root.swatches[i]))
        return true;
    }
    return false;
  }

  // Moves keyboard focus to the previous or next cell in the chain
  function moveFocus(item: Item, forward: bool): void {
    const next = item.nextItemInFocusChain(forward);
    if (next)
      next.forceActiveFocus();
  }

  // One grid cell: a fill that reacts to hover and press, a selection
  // border in the polarity ink of the fill, and a keyboard focus ring
  // drawn outside the cell with a gap. Activation is keyboard- and
  // screen-reader-accessible. The selected cell takes focus when the
  // popover opens (selection is exclusive)
  component PickerCell: Rectangle {
    id: cell

    property color fill: "transparent"
    property bool checked: false
    property int accessibleRole: Accessible.RadioButton
    property string accessibleName: ""

    signal activated()

    function activate(): void {
      cell.activated();
    }

    activeFocusOnTab: true
    border.color: cell.checked ? ZrythmTheme.polarityTextColor(cell.fill) : "transparent"
    border.width: 2
    color: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(
      cell.fill, hoverHandler.hovered, false, tapHandler.pressed)
    focus: cell.checked
    implicitHeight: 24
    implicitWidth: 24
    radius: ZrythmTheme.textFieldRadius

    Accessible.role: cell.accessibleRole
    Accessible.checked: cell.checked
    Accessible.name: cell.accessibleName
    Accessible.onPressAction: cell.activate()

    Rectangle {
      anchors.fill: parent
      anchors.margins: -3
      border.color: palette.highlight
      border.width: 2
      color: "transparent"
      radius: cell.radius + 3
      visible: cell.activeFocus
      z: -1
    }

    HoverHandler {
      id: hoverHandler
      cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
      id: tapHandler
      onTapped: cell.activate()
    }

    Keys.onLeftPressed: (event) => root.moveFocus(cell, false)
    Keys.onRightPressed: (event) => root.moveFocus(cell, true)
    Keys.onDownPressed: (event) => root.moveFocus(cell, true)
    Keys.onUpPressed: (event) => root.moveFocus(cell, false)
    Keys.onReturnPressed: (event) => cell.activate()
    Keys.onEnterPressed: (event) => cell.activate()
    Keys.onSpacePressed: (event) => cell.activate()
  }

  contentItem: GridLayout {
    columns: 7
    columnSpacing: 4
    rowSpacing: 4

    // Default cell: live preview of what an unset subject uses
    PickerCell {
      id: defaultCell

      accessibleName: qsTr("Default color")
      checked: !root.useColor
      clip: true
      fill: root.defaultColor

      onActivated: {
        root.unsetRequested();
        root.close();
      }

      // Slash marking the default cell
      Rectangle {
        anchors.centerIn: parent
        antialiasing: true
        color: ZrythmTheme.polarityTextColor(defaultCell.fill)
        height: 2
        rotation: 45
        width: parent.width * 1.5
      }
    }

    Repeater {
      id: swatchRepeater

      model: root.swatches

      delegate: PickerCell {
        id: swatch

        required property color modelData
        required property int index

        accessibleName: qsTr("Color swatch %1").arg(swatch.index + 1)
        checked: root.useColor
                 && Qt.colorEqual(root.currentColor, swatch.modelData)
        fill: swatch.modelData

        onActivated: {
          root.colorSelected(swatch.modelData);
          root.close();
        }
      }
    }

    // Custom color: opens the system dialog
    PickerCell {
      id: customCell

      readonly property bool customActive: root.useColor
        && !root.usesSwatchColor()

      accessibleName: qsTr("Custom color")
      accessibleRole: Accessible.PushButton
      checked: customActive
      fill: palette.button

      onActivated: {
        colorDialog.selectedColor = root.useColor
          ? root.currentColor : root.defaultColor;
        colorDialog.open();
      }

      GridLayout {
        anchors.centerIn: parent
        columns: 2
        columnSpacing: 1
        rowSpacing: 1

        Repeater {
          model: [
            TrackColors.defaultColor(Track.Master, ZrythmTheme.darkMode),
            TrackColors.defaultColor(Track.Instrument, ZrythmTheme.darkMode),
            TrackColors.defaultColor(Track.Chord, ZrythmTheme.darkMode),
            TrackColors.defaultColor(Track.Folder, ZrythmTheme.darkMode),
          ]

          delegate: Rectangle {
            id: quadrant

            required property color modelData

            color: quadrant.modelData
            Layout.preferredHeight: 8
            Layout.preferredWidth: 8
            radius: 1
          }
        }
      }
    }
  }

  ColorDialog {
    id: colorDialog

    objectName: "colorDialog"
    title: qsTr("Choose Color")

    onAccepted: {
      root.colorSelected(colorDialog.selectedColor);
      root.close();
    }
  }
}
