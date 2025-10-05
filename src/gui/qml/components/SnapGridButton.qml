// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

/**
 * Snap/Grid settings button widget.
 *
 * Displays current snap settings and provides access to snap configuration
 * through a popup dialog.
 */
Button {
  id: root

  // Required property: the SnapGrid model
  required property SnapGrid snapGrid

  display: AbstractButton.TextBesideIcon

  // Button appearance
  icon.source: "qrc:/icons/snap-to-grid.svg"

  // Button text based on current snap settings
  text: {
    if (!snapGrid.snapToGrid) {
      return qsTr("Off");
    }

    const snapStr = snapGrid.snapString;

    switch (snapGrid.lengthType) {
    case SnapGrid.NoteLengthType.Link:
      return snapStr + " - ↔";
    case SnapGrid.NoteLengthType.LastObject:
      return qsTr("%1 - Last object").arg(snapStr);
    case SnapGrid.NoteLengthType.Custom:
      const defaultStr = SnapGrid.stringize_length_and_type(snapGrid.default_note_length, snapGrid.default_note_type);
      return qsTr("%1 - %2").arg(snapStr).arg(defaultStr);
    default:
      return snapStr;
    }
  }

  // Popup dialog for snap settings
  onClicked: snapDialog.open()

  // Tooltip text
  ToolTip {
    text: qsTr("Snap/Grid Settings")
  }

  Popup {
    id: snapDialog

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true
    height: 500
    modal: true
    width: 400

    ColumnLayout {
      anchors.fill: parent
      spacing: 10

      // Header
      Label {
        Layout.alignment: Qt.AlignHCenter
        font.bold: true
        font.pixelSize: 16
        text: qsTr("Snap/Grid Settings")
      }

      // Position Snap section
      GroupBox {
        Layout.fillWidth: true
        title: qsTr("Position Snap")

        ColumnLayout {
          anchors.fill: parent
          spacing: 5

          // Snap to grid toggle
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Snap to Grid")
            }

            Switch {
              checked: snapGrid.snapToGrid

              onToggled: snapGrid.snapToGrid = checked
            }
          }

          // Adaptive snap
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Adaptive Snap")
            }

            Switch {
              checked: snapGrid.snapAdaptive
              enabled: snapGrid.snapToGrid

              onToggled: snapGrid.snapAdaptive = checked
            }
          }

          // Snap length selection
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Snap Length")
            }

            ComboBox {
              currentIndex: {
                switch (snapGrid.snapNoteLength) {
                case SnapGrid.NoteLength.Bar:
                  return 0;
                case SnapGrid.NoteLength.Note_1_1:
                  return 1;
                case SnapGrid.NoteLength.Note_1_2:
                  return 2;
                case SnapGrid.NoteLength.Note_1_4:
                  return 3;
                case SnapGrid.NoteLength.Note_1_8:
                  return 4;
                case SnapGrid.NoteLength.Note_1_16:
                  return 5;
                case SnapGrid.NoteLength.Note_1_32:
                  return 6;
                case SnapGrid.NoteLength.Note_1_64:
                  return 7;
                case SnapGrid.NoteLength.Note_1_128:
                  return 8;
                default:
                  return 3;
                }
              }
              enabled: snapGrid.snapToGrid && !snapGrid.snapAdaptive
              model: [qsTr("Bar"), qsTr("1/1"), qsTr("1/2"), qsTr("1/4"), qsTr("1/8"), qsTr("1/16"), qsTr("1/32"), qsTr("1/64"), qsTr("1/128")]

              onActivated: {
                const lengths = [SnapGrid.NoteLength.Bar, SnapGrid.NoteLength.Note_1_1, SnapGrid.NoteLength.Note_1_2, SnapGrid.NoteLength.Note_1_4, SnapGrid.NoteLength.Note_1_8, SnapGrid.NoteLength.Note_1_16, SnapGrid.NoteLength.Note_1_32, SnapGrid.NoteLength.Note_1_64, SnapGrid.NoteLength.Note_1_128];
                snapGrid.snapNoteLength = lengths[currentIndex];
              }
            }
          }

          // Note type selection
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Note Type")
            }

            ComboBox {
              currentIndex: {
                switch (root.snapGrid.snapNoteType) {
                case SnapGrid.Normal:
                  return 0;
                case SnapGrid.Triplet:
                  return 1;
                case SnapGrid.Dotted:
                  return 2;
                default:
                  return 0;
                }
              }
              enabled: snapGrid.snapToGrid && !snapGrid.snapAdaptive
              model: [qsTr("Normal"), qsTr("Triplet"), qsTr("Dotted")]

              onActivated: {
                const types = [SnapGrid.NoteType.Normal, SnapGrid.NoteType.Triplet, SnapGrid.NoteType.Dotted];
                snapGrid.snapNoteType = types[currentIndex];
              }
            }
          }

          // Keep offset
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Keep Offset")
            }

            Switch {
              checked: snapGrid.keepOffset
              enabled: snapGrid.snapToGrid

              onToggled: snapGrid.keepOffset = checked
            }
          }

          // Snap to events
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Snap to Events")
            }

            Switch {
              checked: snapGrid.snapToEvents
              enabled: snapGrid.snapToGrid

              onToggled: snapGrid.snapToEvents = checked
            }
          }
        }
      }

      // Default Object Length section
      GroupBox {
        Layout.fillWidth: true
        title: qsTr("Default Object Length")

        ColumnLayout {
          anchors.fill: parent
          spacing: 5

          // Object length type
          RowLayout {
            Label {
              Layout.fillWidth: true
              text: qsTr("Length Type")
            }

            ComboBox {
              currentIndex: {
                switch (root.snapGrid.lengthType) {
                case SnapGrid.NoteLengthType.Link:
                  return 0;
                case SnapGrid.NoteLengthType.LastObject:
                  return 1;
                case SnapGrid.NoteLengthType.Custom:
                  return 2;
                default:
                  return 0;
                }
              }
              enabled: snapGrid.snapToGrid
              model: [qsTr("Link to snap"), qsTr("Last object"), qsTr("Custom")]

              onActivated: {
                const types = [SnapGrid.NoteLengthType.Link, SnapGrid.NoteLengthType.LastObject, SnapGrid.NoteLengthType.Custom];
                snapGrid.lengthType = types[currentIndex];
              }
            }
          }

          // Custom length selection (only visible when Custom is selected)
          RowLayout {
            visible: snapGrid.lengthType === SnapGrid.NoteLengthType.Custom

            Label {
              Layout.fillWidth: true
              text: qsTr("Custom Length")
            }

            ComboBox {
              currentIndex: {
                switch (snapGrid.default_note_length) {
                case SnapGrid.NoteLength.Bar:
                  return 0;
                case SnapGrid.NoteLength.Note_1_1:
                  return 1;
                case SnapGrid.NoteLength.Note_1_2:
                  return 2;
                case SnapGrid.NoteLength.Note_1_4:
                  return 3;
                case SnapGrid.NoteLength.Note_1_8:
                  return 4;
                case SnapGrid.NoteLength.Note_1_16:
                  return 5;
                case SnapGrid.NoteLength.Note_1_32:
                  return 6;
                case SnapGrid.NoteLength.Note_1_64:
                  return 7;
                case SnapGrid.NoteLength.Note_1_128:
                  return 8;
                default:
                  return 3;
                }
              }
              enabled: snapGrid.snapToGrid && snapGrid.lengthType === SnapGrid.NoteLengthType.Custom
              model: [qsTr("Bar"), qsTr("1/1"), qsTr("1/2"), qsTr("1/4"), qsTr("1/8"), qsTr("1/16"), qsTr("1/32"), qsTr("1/64"), qsTr("1/128")]

              onActivated: {
                const lengths = [SnapGrid.NoteLength.Bar, SnapGrid.NoteLength.Note_1_1, SnapGrid.NoteLength.Note_1_2, SnapGrid.NoteLength.Note_1_4, SnapGrid.NoteLength.Note_1_8, SnapGrid.NoteLength.Note_1_16, SnapGrid.NoteLength.Note_1_32, SnapGrid.NoteLength.Note_1_64, SnapGrid.NoteLength.Note_1_128];
                snapGrid.default_note_length = lengths[currentIndex];
              }
            }
          }

          // Custom type selection (only visible when Custom is selected)
          RowLayout {
            visible: snapGrid.lengthType === SnapGrid.NoteLengthType.Custom

            Label {
              Layout.fillWidth: true
              text: qsTr("Custom Type")
            }

            ComboBox {
              currentIndex: {
                switch (snapGrid.default_note_type) {
                case SnapGrid.NoteType.Normal:
                  return 0;
                case SnapGrid.NoteType.Triplet:
                  return 1;
                case SnapGrid.NoteType.Dotted:
                  return 2;
                default:
                  return 0;
                }
              }
              enabled: snapGrid.snapToGrid && snapGrid.lengthType === SnapGrid.NoteLengthType.Custom
              model: [qsTr("Normal"), qsTr("Triplet"), qsTr("Dotted")]

              onActivated: {
                const types = [SnapGrid.NoteType.Normal, SnapGrid.NoteType.Triplet, SnapGrid.NoteType.Dotted];
                snapGrid.default_note_type = types[currentIndex];
              }
            }
          }
        }
      }

      // Close button
      Button {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Close")

        onClicked: snapDialog.close()
      }
    }
  }
}
