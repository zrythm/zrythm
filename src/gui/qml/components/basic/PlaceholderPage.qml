// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ZrythmStyle

// Full-bleed placeholder page: fills its parent and centers an icon, title,
// description and optional action, capping the column width to a fraction of
// the page for readability.
Item {
  id: root

  property alias action: actionButton.action
  property alias buttonText: actionButton.text
  property alias description: descriptionLabel.text
  property alias icon: placeholderIcon
  property alias title: titleLabel.text
  // Whether to tint the icon with the text color. Leave off for
  // full-color icons (e.g. the logo); symbolic icons want it on.
  property bool tintIcon: true

  // Fill the parent when placed as a plain child; layout parents (e.g.
  // StackLayout) override these sizes where they manage the item.
  height: parent?.height ?? 0
  width: parent?.width ?? 0

  ColumnLayout {
    id: column

    anchors.centerIn: root
    spacing: 10
    width: Math.min(root.width - 40, root.width * 0.8)

    IconImage {
      id: placeholderIcon

      Layout.alignment: Qt.AlignHCenter
      Layout.bottomMargin: 16
      Layout.preferredHeight: 128
      Layout.preferredWidth: 128
      color: root.tintIcon ? root.palette.buttonText : "transparent"
      fillMode: Image.PreserveAspectFit
      sourceSize.height: height
      // these ensure SVGs are scaled and not stretched
      sourceSize.width: width
      visible: status === Image.Ready
    }

    Label {
      id: titleLabel

      Layout.fillWidth: true
      font.bold: true
      font.pointSize: 20
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.WordWrap
    }

    Label {
      id: descriptionLabel

      Layout.fillWidth: true
      font.pointSize: 10.5
      horizontalAlignment: Text.AlignHCenter
      text: ""
      visible: text !== ""
      wrapMode: Text.WordWrap

      onLinkActivated: link => {
        console.log("Link clicked:", link);
        Qt.openUrlExternally(link);
      }

      MouseArea {
        acceptedButtons: Qt.NoButton
        anchors.fill: parent
        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
      }
    }

    Button {
      id: actionButton

      Layout.alignment: Qt.AlignHCenter
      Layout.topMargin: 10
      highlighted: true
      visible: action || text
    }
  }
}
