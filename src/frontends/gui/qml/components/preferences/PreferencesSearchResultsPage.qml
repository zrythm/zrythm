// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle

ScrollView {
  id: root

  signal resultSelected(int categoryIndex)

  property var entries: []

  clip: true
  contentWidth: availableWidth

  ListView {
    id: resultsList

    anchors.fill: parent
    boundsBehavior: Flickable.StopAtBounds
    model: root.entries.length
    spacing: ZrythmTheme.buttonPadding

    Component.onCompleted: currentIndex = 0

    delegate: ActionRow {
      id: resultDelegate

      required property int index

      readonly property var entry: root.entries[resultDelegate.index] ?? null

      highlighted: resultsList.activeFocus && resultsList.currentIndex === resultDelegate.index
      interactive: true
      subtitle: resultDelegate.entry ? qsTr("%1 → %2").arg(resultDelegate.entry.categoryTitle).arg(resultDelegate.entry.group) : ""
      title: resultDelegate.entry?.title ?? ""
      width: resultsList.width

      onClicked: {
        if (resultDelegate.entry !== null)
          root.resultSelected(resultDelegate.entry.categoryIndex);
      }
    }

    header: Label {
      color: palette.text
      font: ZrythmTheme.smallTextFont
      opacity: 0.55
      padding: 8
      text: root.entries.length > 0 ? qsTr("%1 matching settings").arg(root.entries.length) : qsTr("No matching settings")
      width: resultsList.width
    }

    Keys.onEnterPressed: root.selectCurrentResult()
    Keys.onReturnPressed: root.selectCurrentResult()
  }

  function focusFirstResult(): void {
    resultsList.forceActiveFocus();
    resultsList.currentIndex = 0;
  }

  function selectCurrentResult(): void {
    if (resultsList.currentIndex >= 0 && resultsList.currentIndex < root.entries.length)
      root.resultSelected(root.entries[resultsList.currentIndex].categoryIndex);
  }
}
