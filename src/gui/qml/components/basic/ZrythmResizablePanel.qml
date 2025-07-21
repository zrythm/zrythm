// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle 1.0

Pane {
  id: root

  property bool collapsed: false
  property int collapsedSize: 30
  property alias content: contentLoader.sourceComponent
  property string title: "Panel"
  property bool vertical: true

  implicitHeight: vertical ? (collapsed ? collapsedSize : contentLoader.implicitHeight + collapsedSize) : parent.height
  implicitWidth: vertical ? parent.width : (collapsed ? collapsedSize : contentLoader.implicitWidth + collapsedSize)
  padding: 0

  background: Rectangle {
    border.color: "#1C1C1C"
    border.width: 1
    color: "#2C2C2C"
  }

  RowLayout {
    anchors.fill: parent
    spacing: 0

    Rectangle {
      id: header

      Layout.fillHeight: true
      Layout.preferredHeight: root.vertical ? root.collapsedSize : parent.height
      Layout.preferredWidth: root.vertical ? parent.width : root.collapsedSize
      color: "#3C3C3C"

      RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Label {
          Layout.alignment: root.vertical ? Qt.AlignVCenter : Qt.AlignBottom
          Layout.fillWidth: true
          color: "white"
          rotation: root.vertical ? 0 : -90
          text: root.title
        }

        Button {
          Layout.alignment: Qt.AlignRight | Qt.AlignTop
          implicitHeight: 20
          implicitWidth: 20
          text: root.collapsed ? "+" : "-"

          onClicked: root.collapsed = !root.collapsed
        }
      }
    }

    Loader {
      id: contentLoader

      Layout.fillHeight: true
      Layout.fillWidth: true
      visible: !root.collapsed
    }
  }
}
