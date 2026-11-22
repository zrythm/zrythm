// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  property int labelHeights: 24
  property int rightMargin: 4
  required property TempoMap tempoMap

  spacing: 1

  TempoMapLaneHeader {
    Layout.fillWidth: true
    Layout.preferredHeight: root.labelHeights
    Layout.rightMargin: root.rightMargin
    text: qsTr("Tempo")
  }

  TempoMapLaneHeader {
    Layout.fillWidth: true
    Layout.preferredHeight: root.labelHeights
    Layout.rightMargin: root.rightMargin
    text: qsTr("Time Signature")
  }
}
