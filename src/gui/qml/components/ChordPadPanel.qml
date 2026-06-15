// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  // FIXME: find a cleaner way to get a MusicalScale instance for calling static methods
  readonly property MusicalScale _scaleHelper: MusicalScale {
  }
  readonly property ChordPadBank chordPadBank: root.session.uiState.chordPadBank
  required property ChordPresetManager chordPresetManager
  readonly property ChordTrack chordTrack: root.session.project.tracklist.singletonTracks.chordTrack ?? null
  required property ProjectSession session

  spacing: 0

  ChordPadBankOperator {
    id: padOp

    chordPadBank: root.chordPadBank
    undoStack: root.session.undoStack
  }

  ZrythmToolBar {
    Layout.fillWidth: true

    leftItems: [
      ToolButton {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "arrow-pointing-at-line-down-symbolic.svg")
        text: qsTr("Apply Preset")

        onClicked: presetMenu.popup()

        ToolTip {
          text: qsTr("Populate pads from a preset")
        }

        Menu {
          id: presetMenu

          Menu {
            id: fromScaleMenu

            title: qsTr("From Scale")

            Instantiator {
              model: root._scaleHelper.availableScaleTypesWithTriads()

              delegate: Menu {
                id: scaleMenu

                required property var modelData

                title: modelData.display

                Instantiator {
                  model: 12

                  delegate: MenuItem {
                    required property int index

                    text: root._scaleHelper.noteToString(index)

                    onTriggered: padOp.applyPresetFromScale(scaleMenu.modelData.value, index)
                  }

                  onObjectAdded: scaleMenu.addItem(object)
                  onObjectRemoved: scaleMenu.removeItem(object)
                }
              }

              onObjectAdded: fromScaleMenu.addMenu(object)
              onObjectRemoved: fromScaleMenu.removeMenu(object)
            }
          }

          MenuSeparator {
          }

          Instantiator {
            model: root.chordPresetManager.categories()

            delegate: Menu {
              id: categoryMenu

              required property string modelData

              title: modelData

              Instantiator {
                model: root.chordPresetManager.presetsInCategory(categoryMenu.modelData)

                delegate: MenuItem {
                  required property var modelData

                  text: modelData.name

                  onTriggered: padOp.applyPreset(modelData)
                }

                onObjectAdded: categoryMenu.addItem(object)
                onObjectRemoved: categoryMenu.removeItem(object)
              }
            }

            onObjectAdded: presetMenu.addMenu(object)
            onObjectRemoved: presetMenu.removeMenu(object)
          }
        }
      }
    ]
    rightItems: [
      Label {
        text: qsTr("Transpose")
      },
      ToolButton {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "go-up-symbolic.svg")

        onClicked: padOp.transposePads(true)

        ToolTip {
          text: qsTr("Transpose Up")
        }
      },
      ToolButton {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "go-down-symbolic.svg")

        onClicked: padOp.transposePads(false)

        ToolTip {
          text: qsTr("Transpose Down")
        }
      }
    ]
  }

  Flow {
    Layout.fillWidth: true
    padding: ZrythmTheme.buttonPadding
    spacing: ZrythmTheme.buttonPadding

    Repeater {
      model: root.chordPadBank

      delegate: Item {
        id: padWrapper

        required property ChordDescriptor chordDescriptor
        required property int index

        height: 64
        width: 120

        // Drag-to-editor support: declarative Drag properties + DragHandler.
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.keys: ["application/x-zrythm-chord"]
        Drag.mimeData: padWrapper.chordDescriptor ? {
          "application/x-zrythm-chord": JSON.stringify ({
            root: padWrapper.chordDescriptor.rootNote,
            type: padWrapper.chordDescriptor.chordType,
            accent: padWrapper.chordDescriptor.chordAccent,
            hasBass: padWrapper.chordDescriptor.hasBass,
            bass: padWrapper.chordDescriptor.hasBass ? padWrapper.chordDescriptor.bassNote : 0,
            inversion: padWrapper.chordDescriptor.inversion
          })
        } : ({})

        HoverHandler {
          id: padHoverHandler
        }

        DragHandler {
          id: padDragHandler
          target: null

          onActiveChanged: {
            padWrapper.Drag.active = active;
          }
        }

        AbstractButton {
          id: padButton

          anchors.fill: parent

          background: Rectangle {
            border.color: root.palette.mid
            border.width: 1
            color: padButton.pressed ? root.palette.highlight : root.palette.button
            radius: ZrythmTheme.toolButtonRadius
          }
          contentItem: Item {
            Label {
              anchors.centerIn: parent
              color: padButton.pressed ? root.palette.highlightedText : root.palette.text
              font.bold: padButton.pressed
              horizontalAlignment: Text.AlignHCenter
              text: padWrapper.chordDescriptor ? padWrapper.chordDescriptor.displayName : ""
              verticalAlignment: Text.AlignVCenter
            }
          }

          onCanceled: {
            if (root.chordTrack && padWrapper.chordDescriptor)
              root.chordTrack.auditionChord(padWrapper.chordDescriptor, false);
          }
          onPressed: {
            if (root.chordTrack && padWrapper.chordDescriptor)
              root.chordTrack.auditionChord(padWrapper.chordDescriptor, true);
          }
          onReleased: {
            if (root.chordTrack && padWrapper.chordDescriptor)
              root.chordTrack.auditionChord(padWrapper.chordDescriptor, false);
          }
        }

        // Hover actions button overlay (single corner button)
        Item {
          id: hoverOverlay

          anchors.fill: parent
          visible: padHoverHandler.hovered

          // More-actions button (top-right)
          ToolButton {
            font.pixelSize: 14
            opacity: 0.7
            text: "\u22ef"

            onClicked: padActionsMenu.popup()

            anchors {
              margins: 2
              right: parent.right
              top: parent.top
            }
          }
        }

        // Right-click anywhere on the pad opens the same actions menu
        TapHandler {
          acceptedButtons: Qt.RightButton

          onTapped: padActionsMenu.popup()
        }

        Menu {
          id: padActionsMenu

          MenuItem {
            text: qsTr("Edit\u2026")

            onTriggered: {
              editDialogLoader.active = true;
              editDialogLoader.item.editIndex = padWrapper.index;
              editDialogLoader.item.applyFromDescriptor(padWrapper.chordDescriptor);
              editDialogLoader.item.open();
            }
          }

          MenuItem {
            text: qsTr("Remove")

            onTriggered: padOp.removePad(padWrapper.index)
          }

          MenuSeparator {
          }

          MenuItem {
            text: qsTr("Invert Backward")

            onTriggered: {
              if (padWrapper.chordDescriptor) {
                const d = padWrapper.chordDescriptor;
                padOp.editPad(padWrapper.index, d.rootNote, d.chordType, d.chordAccent, d.inversion - 1, d.hasBass, d.bassNote);
              }
            }
          }

          MenuItem {
            text: qsTr("Invert Forward")

            onTriggered: {
              if (padWrapper.chordDescriptor) {
                const d = padWrapper.chordDescriptor;
                padOp.editPad(padWrapper.index, d.rootNote, d.chordType, d.chordAccent, d.inversion + 1, d.hasBass, d.bassNote);
              }
            }
          }

          MenuSeparator {
          }

          MenuItem {
            text: qsTr("Transpose Up")

            onTriggered: {
              if (padWrapper.chordDescriptor) {
                const d = padWrapper.chordDescriptor;
                padOp.editPad(padWrapper.index, (d.rootNote + 1) % 12, d.chordType, d.chordAccent, d.inversion, d.hasBass, d.bassNote);
              }
            }
          }

          MenuItem {
            text: qsTr("Transpose Down")

            onTriggered: {
              if (padWrapper.chordDescriptor) {
                const d = padWrapper.chordDescriptor;
                padOp.editPad(padWrapper.index, (d.rootNote + 11) % 12, d.chordType, d.chordAccent, d.inversion, d.hasBass, d.bassNote);
              }
            }
          }
        }
      }
    }

    // "Add" pad at the end
    AbstractButton {
      height: 64
      width: 120

      background: Rectangle {
        border.color: root.palette.mid
        border.width: 1
        color: "transparent"
        radius: ZrythmTheme.toolButtonRadius
      }
      contentItem: Label {
        color: root.palette.mid
        font.pixelSize: 24
        horizontalAlignment: Text.AlignHCenter
        text: "+"
        verticalAlignment: Text.AlignVCenter
      }

      onClicked: padOp.addPad(MusicalScale.MusicalNote.C, MusicalScale.ChordType.Major, MusicalScale.ChordAccent.None, 0)
    }
  }

  Loader {
    id: editDialogLoader

    active: false

    sourceComponent: ChordSelectorDialog {
      id: editDialog

      property int editIndex: -1

      chordTrack: root.chordTrack
      playheadTicks: root.session.project.transport.playhead.ticks

      onAccepted: {
        padOp.editPad(editDialog.editIndex, editDialog.tempRootNote, editDialog.tempChordType, editDialog.tempChordAccent, editDialog.tempInversion, editDialog.tempHasBass, editDialog.tempBassNote);
      }
      onClosed: {
        editDialogLoader.active = false;
      }
    }
  }
}
