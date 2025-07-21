// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import ZrythmStyle 1.0

ColumnLayout {
  id: root

  property alias action: actionButton.action
  property alias buttonText: actionButton.text
  property alias description: descriptionLabel.text
  property alias icon: placeholderIcon
  property alias title: titleLabel.text

  spacing: 10
  width: Math.min(parent.width - 40, parent.width * 0.8)

  IconImage {
    id: placeholderIcon

    Layout.alignment: Qt.AlignHCenter
    Layout.bottomMargin: 16
    Layout.preferredHeight: 128
    Layout.preferredWidth: 128
    color: root.palette.buttonText
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
