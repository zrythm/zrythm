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
  property alias frameContentItem: frame.contentItem
  property alias icon: expandButton.icon
  readonly property int radius: 4
  property string title: "Expander"
  property bool vertical: false
  // Optional component placed at the header's trailing edge (e.g. an
  // add button); instantiated only when set
  property Component headerAction: null

  padding: 4

  contentItem: ColumnLayout {
    spacing: 0

    Button {
      id: expandButton

      readonly property int iconSize: 16

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
      checkable: true
      checked: root.expanded
      hoverEnabled: true
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
        spacing: 8

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
          Layout.preferredHeight: 14
          Layout.preferredWidth: 14
          color: expandButton.palette.buttonText
          rotation: root.expanded ? 90 : 0
          source: "qrc:/qt/qml/Zrythm/icons/lucide/chevron-right.svg"
          sourceSize: Qt.size(14, 14)

          Behavior on rotation {
            animation: ZrythmTheme.propertyAnimation
          }
        }
      }

      onToggled: root.expanded = checked
    }

    Frame {
      id: frame

      Layout.fillHeight: root.vertical
      Layout.fillWidth: !root.vertical
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
          color: palette.button
          width: 2
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
