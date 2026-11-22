// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import ZrythmArrangement
import ZrythmGui
import ZrythmStyle

/**
 * The audio clip's transform entry stack, listed in application order.
 * Selecting a row reveals the controls that move the entry within the
 * stack; the bake button materializes the whole stack into the clip's
 * audio.
 */
ColumnLayout {
  id: root

  required property AudioClip audioClip
  required property ArrangerObjectSelectionOperator selectionOperator

  /** Index of the selected entry, or -1 when none. */
  required property int selectedTransformEntryIndex

  signal selectionChanged (int index)

  spacing: 0

  AudioTransformEntryListModel {
    id: entryListModel

    clip: root.audioClip
  }

  onSelectedTransformEntryIndexChanged: {
    entryList.currentIndex = root.selectedTransformEntryIndex;
  }

  RowLayout {
    Layout.topMargin: 4
    Layout.leftMargin: 8
    Layout.rightMargin: 4
    Layout.fillWidth: true
    spacing: 4

    Label {
      text: qsTr ("Transform Stack")
      font: ZrythmTheme.semiBoldTextFont
    }

    Item {
      Layout.fillWidth: true
    }

    Label {
      font: ZrythmTheme.smallTextFont
      color: ZrythmTheme.placeholderTextColor
      text: entryListModel.count
      visible: entryListModel.count > 0

      Accessible.role: Accessible.StaticText
      Accessible.name: qsTr ("%1 transform entries").arg (entryListModel.count)
    }
  }

  Rectangle {
    Layout.topMargin: 4
    Layout.fillWidth: true
    implicitHeight: 1
    color: ZrythmTheme.gunmetalColor
  }

  Label {
    Layout.fillWidth: true
    Layout.topMargin: 4
    Layout.leftMargin: 8
    Layout.rightMargin: 8
    visible: entryListModel.count === 0
    font: ZrythmTheme.smallTextFont
    color: ZrythmTheme.placeholderTextColor
    text: qsTr ("No transform entries. Add one from the Functions menu.")
    wrapMode: Text.WordWrap

    Accessible.role: Accessible.StaticText
  }

  ListView {
    id: entryList

    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    keyNavigationEnabled: true
    model: entryListModel
    currentIndex: -1
    visible: entryListModel.count > 0

    Accessible.name: qsTr ("Transform Stack")
    Accessible.role: Accessible.List

    ScrollBar.vertical: ScrollBar {}

    Component.onCompleted: {
      entryList.currentIndex = root.selectedTransformEntryIndex;
    }
    onCurrentIndexChanged: {
      if (currentIndex !== root.selectedTransformEntryIndex)
        root.selectionChanged (currentIndex);
    }

      delegate: ItemDelegate {
        id: entryRow

        required property int index
        required property audioTransformEntry entry

        readonly property bool selected: root.selectedTransformEntryIndex === entryRow.index
        readonly property color kindColor: AudioTransformKindColors.kindColor (entryRow.entry.kindId)

        width: ListView.view.width
        implicitHeight: ZrythmTheme.buttonHeight
        focusPolicy: Qt.StrongFocus
        highlighted: entryRow.selected
        padding: 0
        leftPadding: 8
        rightPadding: 4

        Accessible.name: {
          let name = entryRow.entry.label;
          if (entryRow.entry.detail !== "")
            name += ", " + entryRow.entry.detail;
          if (!entryRow.entry.enabled)
            name += ", " + qsTr ("Bypassed");
          return name;
        }
        Accessible.role: Accessible.ListItem
        Accessible.selectable: true
        Accessible.selected: entryRow.selected

        onClicked: {
          root.selectionChanged (entryRow.selected ? -1 : entryRow.index);
        }

        background: Rectangle {
          radius: ZrythmTheme.textFieldRadius
          opacity: entryRow.entry.enabled ? 1 : ZrythmTheme.disabledOpacityFactor
          color: entryRow.selected ? Qt.alpha (ZrythmTheme.primaryColor, 0.12) : "transparent"
          border.width: entryRow.selected || entryRow.activeFocus ? 1 : 0
          border.color: entryRow.activeFocus ? entryRow.palette.highlight : Qt.alpha (ZrythmTheme.primaryColor, 0.5)

          Rectangle {
            x: 0
            width: 3
            height: parent.height - 8
            radius: 1.5
            anchors.verticalCenter: parent.verticalCenter
            color: entryRow.kindColor
          }
        }

        contentItem: RowLayout {
          spacing: 4
          opacity: entryRow.entry.enabled ? 1 : ZrythmTheme.disabledOpacityFactor

          Label {
            text: entryRow.entry.label
            font: ZrythmTheme.semiBoldTextFont
            color: ZrythmTheme.textColor
          }

          // Always takes part in the layout (renders nothing when there
          // is no detail) so the row buttons line up on every row.
          Label {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            font: ZrythmTheme.xSmallTextFont
            color: ZrythmTheme.placeholderTextColor
            elide: Text.ElideRight
            text: entryRow.entry.detail
          }

          ToolButton {
            Accessible.name: entryRow.entry.enabled ? qsTr ("Bypass Entry") : qsTr ("Enable Entry")
            icon.source: ResourceManager.getIconUrl (
              "gnome-icon-library",
              entryRow.entry.enabled
                ? "eye-open-negative-filled-symbolic.svg"
                : "eye-not-looking-symbolic.svg")

            ToolTip {
              text: entryRow.entry.enabled ? qsTr ("Bypass Entry") : qsTr ("Enable Entry")
            }

            onClicked: {
              root.selectionOperator.setAudioTransformEntryEnabled (
                root.audioClip, entryRow.index, !entryRow.entry.enabled);
            }
          }

          ToolButton {
            Accessible.name: qsTr ("Remove Entry")
            icon.source: ResourceManager.getIconUrl (
              "gnome-icon-library", "cross-small-circle-outline-symbolic.svg")

            ToolTip {
              text: qsTr ("Remove Entry")
            }

            onClicked: {
              const removed = entryRow.index;
              const selected = root.selectedTransformEntryIndex;
              if (
                !root.selectionOperator.removeAudioTransformEntry (
                  root.audioClip, removed))
                return;
              if (removed === selected)
                root.selectionChanged (-1);
              else if (removed < selected)
                root.selectionChanged (selected - 1);
            }
          }

          // Stack placement controls for the selected entry. The wrapper
          // always takes part in the layout so the row buttons keep their
          // positions; the buttons only exist while the entry is selected.
          Item {
            id: stackControls

            Layout.preferredWidth: entryRow.selected ? 52 : 0
            Layout.fillHeight: true
            clip: true

            ToolButton {
              id: moveUpButton

              visible: entryRow.selected
              Accessible.name: qsTr ("Move Entry Up")
              anchors.right: moveDownButton.left
              anchors.rightMargin: 4
              anchors.verticalCenter: parent.verticalCenter
              icon.source: ResourceManager.getIconUrl (
                "gnome-icon-library", "pan-up-symbolic.svg")
              enabled: entryRow.index > 0

              ToolTip {
                text: qsTr ("Move Entry Up")
              }

              onClicked: {
                const target = entryRow.index - 1;
                if (
                  root.selectionOperator.moveAudioTransformEntry (
                    root.audioClip, entryRow.index, target))
                  root.selectionChanged (target);
              }
            }

            ToolButton {
              id: moveDownButton

              visible: entryRow.selected
              Accessible.name: qsTr ("Move Entry Down")
              anchors.right: parent.right
              anchors.verticalCenter: parent.verticalCenter
              icon.source: ResourceManager.getIconUrl (
                "gnome-icon-library", "pan-down-symbolic.svg")
              enabled: entryRow.index < entryListModel.count - 1

              ToolTip {
                text: qsTr ("Move Entry Down")
              }

              onClicked: {
                const target = entryRow.index + 1;
                if (
                  root.selectionOperator.moveAudioTransformEntry (
                    root.audioClip, entryRow.index, target))
                  root.selectionChanged (target);
              }
            }
          }
        }
      }
    }

  Button {
    Layout.margins: 4
    Layout.fillWidth: true
    visible: entryListModel.count > 0
    text: qsTr ("Bake")

    ToolTip {
      text: qsTr ("Bake Transform Entries")
    }

    onClicked: {
      if (root.selectionOperator.bakeAudioClip (root.audioClip))
        root.selectionChanged (-1);
    }
  }
}
