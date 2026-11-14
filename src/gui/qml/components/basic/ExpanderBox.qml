// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick.Layouts
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import ZrythmStyle

Control {
  id: root

  property bool expanded: true
  // Dense variant for mixer strips: a slim header, no frame border and
  // no frame padding
  property bool compact: false
  // When true, `expanded` is driven from outside: the header only
  // reports user toggles through expandedByUser() instead of writing
  // `expanded` itself. Hosts use this to fold a level across many
  // expanders at once (mixer sections).
  property bool externalExpandedState: false
  property alias frameContentItem: frame.contentItem
  property alias icon: expandButton.icon
  readonly property int radius: 4
  // Vertical space the header block occupies while the body is hidden
  // (both paddings plus the header button); hosts reserving blank space
  // for absent sections align with this
  readonly property real headerBlockHeight: 2 * root.padding + expandButton.implicitHeight
  property string title: "Expander"
  property bool vertical: false
  // Optional component placed at the header's trailing edge (e.g. an
  // add button); instantiated only when set
  property Component headerAction: null

  signal expandedByUser(bool expanded)

  padding: root.compact ? 1 : 4

  contentItem: ColumnLayout {
    spacing: 0

    Button {
      id: expandButton

      readonly property int iconSize: root.compact ? 12 : 16

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
      // In shared-state mode the header is a plain click target: `checked`
      // stays a display binding on `expanded` and never gets written by
      // the control, so the fold state cannot desync
      checkable: !root.externalExpandedState
      checked: root.expanded
      font.bold: true
      font.family: ZrythmTheme.fontFamily
      font.pixelSize: root.compact ? 9 : 12
      horizontalPadding: root.compact ? 4 : 6
      hoverEnabled: true
      implicitHeight: root.compact ? ZrythmTheme.compactControlHeight : ZrythmTheme.buttonHeight
      verticalPadding: root.compact ? 1 : 4
      // Constant neutral ink: expansion state is carried by the chevron's
      // rotation, not by icon color changes.
      icon.color: expandButton.palette.buttonText
      icon.height: iconSize
      icon.width: iconSize
      text: root.title

      // Neutral in both states: a disclosure header is not a selection, so
      // the accent stays reserved for the chevron's state and real selections.
      background: Rectangle {
        bottomLeftRadius: root.expanded ? 0 : root.radius
        bottomRightRadius: root.expanded ? 0 : root.radius
        color:
          expandButton.hovered || expandButton.visualFocus || expandButton.down
          ? ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown (
              expandButton.palette.button, expandButton.hovered,
              expandButton.visualFocus, expandButton.down)
          : expandButton.palette.button
        topLeftRadius: root.radius
        topRightRadius: root.radius

        Behavior on color {
          animation: ZrythmTheme.propertyAnimation
        }
      }

      contentItem: RowLayout {
        spacing: root.compact ? 4 : 8

        IconLabel {
          Layout.alignment: Qt.AlignVCenter
          display: IconLabel.IconOnly
          icon: expandButton.icon
          visible: expandButton.icon.source.toString() !== ""
        }

        Label {
          Layout.alignment: Qt.AlignVCenter
          Layout.fillWidth: true
          elide: Text.ElideRight
          font: expandButton.font
          text: expandButton.text
        }

        Loader {
          Layout.alignment: Qt.AlignVCenter
          sourceComponent: root.headerAction
        }

        ColorImage {
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredHeight: root.compact ? 10 : 14
          Layout.preferredWidth: root.compact ? 10 : 14
          color: expandButton.palette.buttonText
          rotation: root.expanded ? 90 : 0
          source: "qrc:/qt/qml/Zrythm/icons/lucide/chevron-right.svg"
          sourceSize: Qt.size(root.compact ? 10 : 14, root.compact ? 10 : 14)

          Behavior on rotation {
            animation: ZrythmTheme.propertyAnimation
          }
        }
      }

      onClicked: {
        if (root.externalExpandedState)
          root.expandedByUser(!root.expanded);
      }

      onToggled: {
        if (!root.externalExpandedState)
          root.expanded = expandButton.checked;
      }
    }

    Frame {
      id: frame

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
      padding: root.compact ? 0 : 12
      visible: root.expanded

      Behavior on Layout.preferredHeight {
        animation: ZrythmTheme.propertyAnimation
      }
      background: Rectangle {
        anchors.fill: parent
        bottomLeftRadius: root.radius
        bottomRightRadius: root.radius
        color.a: 0 // invisible

        border {
          color: root.compact ? "transparent" : palette.button
          width: root.compact ? 0 : 2
        }
      }
    }

    Binding {
      property: "Layout.preferredHeight"
      target: frame
      value: 0
      when: !root.expanded
    }
  }
}
