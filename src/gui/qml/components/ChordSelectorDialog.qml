// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Dialog {
  id: root

  readonly property ChordDescriptor _auditionDesc: ChordDescriptor {
  }
  property bool _editingPad: false
  readonly property MusicalScale _scaleHelper: MusicalScale {
  }
  readonly property var accentSuffixes: ["", "7", "j7", "\u266D9", "9", "\u266F9", "11", "\u266D5/\u266F11", "\u266F5/\u266D13", "6/13"]
  readonly property string chordPreview: {
    let s = root.noteNames[root.tempRootNote];
    s += root.typeSuffixes[root.tempChordType];
    if (root.tempChordAccent > 0)
      s += " " + root.accentSuffixes[root.tempChordAccent];
    if (root.tempHasBass && root.tempBassNote !== root.tempRootNote)
      s += "/" + root.noteNames[root.tempBassNote];
    if (root.tempInversion !== 0)
      s += " i" + root.tempInversion;
    return s;
  }
  property ChordTrack chordTrack: null
  readonly property ScaleObject currentScale: root.chordTrack ? root.chordTrack.scaleAtTicks(root.playheadTicks) : null
  property var diatonicChords: []
  property bool filterToScale: false
  readonly property var noteNames: root._scaleHelper.allNoteNames()
  property double playheadTicks: 0
  property string scaleDisplayName: ""
  property int tempBassNote: MusicalScale.MusicalNote.C
  property int tempChordAccent: MusicalScale.ChordAccent.None
  property int tempChordType: MusicalScale.ChordType.Major
  property bool tempHasBass: false
  property int tempInversion: 0
  property int tempRootNote: MusicalScale.MusicalNote.C
  readonly property var typeSuffixes: ["", "Maj", "min", "dim", "sus4", "sus2", "aug", ""]

  signal chordSelected(int rootNote, int chordType, int chordAccent, int bassNote, int inversion)

  function applyFromDescriptor(desc) {
    if (!desc) {
      root._editingPad = false;
      return resetToDefaults();
    }
    root._editingPad = true;
    root.tempRootNote = desc.rootNote;
    root.tempChordType = desc.chordType;
    root.tempChordAccent = desc.chordAccent;
    root.tempHasBass = desc.hasBass;
    root.tempBassNote = desc.hasBass ? desc.bassNote : MusicalScale.MusicalNote.C;
    root.tempInversion = desc.inversion;
  }

  function auditionCustom() {
    if (!root.chordTrack)
      return;
    root._auditionDesc.rootNote = root.tempRootNote;
    root._auditionDesc.chordType = root.tempChordType;
    root._auditionDesc.chordAccent = root.tempChordAccent;
    root._auditionDesc.inversion = root.tempInversion;
    if (root.tempHasBass) {
      root._auditionDesc.hasBass = true;
      root._auditionDesc.bassNote = root.tempBassNote;
    } else {
      root._auditionDesc.hasBass = false;
    }
    root.chordTrack.previewChord(root._auditionDesc);
  }

  function resetToDefaults() {
    root.tempRootNote = MusicalScale.MusicalNote.C;
    root.tempChordType = MusicalScale.ChordType.Major;
    root.tempChordAccent = MusicalScale.ChordAccent.None;
    root.tempHasBass = false;
    root.tempInversion = 0;
  }

  function updateScaleDependants() {
    if (!root.currentScale || !root.currentScale.scale) {
      root.diatonicChords = [];
      root.scaleDisplayName = "";
      return;
    }
    const scale = root.currentScale.scale;
    root.scaleDisplayName = scale.toString();
    root.diatonicChords = root._scaleHelper.getDiatonicChords(scale.scaleType, scale.rootKey);
  }

  contentWidth: 720
  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Ok | Dialog.Cancel
  title: qsTr("Chord Selector")

  contentItem: ColumnLayout {
    spacing: ZrythmTheme.buttonPadding

    TabBar {
      id: tabBar

      Layout.fillWidth: true

      TabButton {
        text: qsTr("Custom")
      }

      TabButton {
        text: qsTr("Diatonic")
      }
    }

    StackLayout {
      id: stackLayout

      Layout.fillWidth: true
      currentIndex: tabBar.currentIndex

      // Tab 0: Custom
      ColumnLayout {
        spacing: ZrythmTheme.buttonPadding

        RowLayout {
          Layout.fillWidth: true

          Label {
            font.bold: true
            font.pixelSize: 18
            text: root.chordPreview
          }

          Item {
            Layout.fillWidth: true
          }

          CheckBox {
            checked: root.filterToScale
            text: qsTr("Constrain to %1").arg(root.scaleDisplayName)
            visible: root.currentScale !== null

            onToggled: root.filterToScale = checked
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 2
          uniformCellSizes: true

          Repeater {
            model: 12

            Button {
              required property int index

              Layout.fillWidth: true
              Layout.preferredHeight: 28
              checked: index === root.tempRootNote
              enabled: !root.filterToScale || !root.currentScale || !root.currentScale.scale || root.currentScale.scale.containsNote(index)
              flat: true
              text: root.noteNames[index]

              onClicked: {
                root.tempRootNote = index;
                root.auditionCustom();
              }
            }
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 2
          uniformCellSizes: true

          Repeater {
            model: [
              {
                name: qsTr("Maj"),
                value: MusicalScale.ChordType.Major
              },
              {
                name: qsTr("min"),
                value: MusicalScale.ChordType.Minor
              },
              {
                name: qsTr("dim"),
                value: MusicalScale.ChordType.Diminished
              },
              {
                name: qsTr("sus4"),
                value: MusicalScale.ChordType.SuspendedFourth
              },
              {
                name: qsTr("sus2"),
                value: MusicalScale.ChordType.SuspendedSecond
              },
              {
                name: qsTr("aug"),
                value: MusicalScale.ChordType.Augmented
              }
            ]

            delegate: Button {
              required property var modelData

              Layout.fillWidth: true
              Layout.preferredHeight: 28
              checked: modelData.value === root.tempChordType
              enabled: !root.filterToScale || !root.currentScale || !root.currentScale.scale || root.currentScale.scale.containsChord(root.tempRootNote, modelData.value, MusicalScale.ChordAccent.None)
              flat: true
              text: modelData.name

              onClicked: {
                root.tempChordType = modelData.value;
                root.auditionCustom();
              }
            }
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 2
          uniformCellSizes: true

          Repeater {
            model: [
              {
                name: "-",
                value: MusicalScale.ChordAccent.None
              },
              {
                name: "7",
                value: MusicalScale.ChordAccent.Seventh
              },
              {
                name: "j7",
                value: MusicalScale.ChordAccent.MajorSeventh
              },
              {
                name: "\u266D9",
                value: MusicalScale.ChordAccent.FlatNinth
              },
              {
                name: "9",
                value: MusicalScale.ChordAccent.Ninth
              },
              {
                name: "\u266F9",
                value: MusicalScale.ChordAccent.SharpNinth
              },
              {
                name: "11",
                value: MusicalScale.ChordAccent.Eleventh
              },
              {
                name: "\u266D5/\u266F11",
                value: MusicalScale.ChordAccent.FlatFifthSharpEleventh
              },
              {
                name: "\u266F5/\u266D13",
                value: MusicalScale.ChordAccent.SharpFifthFlatThirteenth
              },
              {
                name: "6/13",
                value: MusicalScale.ChordAccent.SixthThirteenth
              }
            ]

            delegate: Button {
              required property var modelData

              Layout.fillWidth: true
              Layout.preferredHeight: 28
              checked: modelData.value === root.tempChordAccent
              enabled: modelData.value === MusicalScale.ChordAccent.None || !root.filterToScale || !root.currentScale || !root.currentScale.scale || root.currentScale.scale.isAccentInScale(root.tempRootNote, root.tempChordType, modelData.value)
              flat: true
              text: modelData.name

              onClicked: {
                root.tempChordAccent = modelData.value;
                root.auditionCustom();
              }
            }
          }
        }

        Label {
          text: qsTr("Bass:")
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 2
          uniformCellSizes: true

          Repeater {
            model: 13

            delegate: Button {
              required property int index
              readonly property bool isNone: index === 0

              Layout.fillWidth: true
              Layout.preferredHeight: 28
              checked: isNone ? !root.tempHasBass : (root.tempHasBass && (index - 1) === root.tempBassNote)
              enabled: isNone || !root.filterToScale || !root.currentScale || !root.currentScale.scale || root.currentScale.scale.containsNote(index - 1)
              flat: true
              text: isNone ? "-" : root.noteNames[index - 1]

              onClicked: {
                if (isNone) {
                  root.tempHasBass = false;
                } else {
                  root.tempHasBass = true;
                  root.tempBassNote = index - 1;
                }
                root.auditionCustom();
              }
            }
          }
        }

        RowLayout {
          spacing: ZrythmTheme.buttonPadding

          Label {
            text: qsTr("Inv:")
          }

          Button {
            flat: true
            text: "\u25C0"

            onClicked: {
              root.tempInversion = root.tempInversion - 1;
              root.auditionCustom();
            }
          }

          Label {
            Layout.minimumWidth: 24
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            text: root.tempInversion
          }

          Button {
            flat: true
            text: "\u25B6"

            onClicked: {
              root.tempInversion = root.tempInversion + 1;
              root.auditionCustom();
            }
          }
        }
      }

      // Tab 1: Diatonic
      ColumnLayout {
        spacing: ZrythmTheme.buttonPadding

        Label {
          font.bold: true
          font.pixelSize: 16
          text: root.scaleDisplayName || qsTr("No scale at playhead")
        }

        Label {
          Layout.fillWidth: true
          color: root.palette.placeholderText
          text: qsTr("Add a scale to the chord track to see diatonic chords here.")
          visible: !root.currentScale
          wrapMode: Text.WordWrap
        }

        Label {
          Layout.fillWidth: true
          color: root.palette.placeholderText
          text: qsTr("No diatonic chord presets available for this scale type. Use the Custom tab to build chords manually.")
          visible: root.currentScale && root.diatonicChords.length === 0
          wrapMode: Text.WordWrap
        }

        Flow {
          Layout.fillWidth: true
          spacing: ZrythmTheme.buttonPadding
          visible: root.diatonicChords.length > 0

          Repeater {
            model: root.diatonicChords

            delegate: Button {
              id: diatonicBtn

              required property var modelData

              checked: modelData.rootNote === root.tempRootNote && modelData.chordType === root.tempChordType && root.tempChordAccent === MusicalScale.ChordAccent.None
              flat: true
              height: 48
              width: 100

              contentItem: ColumnLayout {
                Label {
                  Layout.fillWidth: true
                  color: diatonicBtn.checked ? root.palette.highlightedText : root.palette.text
                  font.pixelSize: 9
                  horizontalAlignment: Text.AlignHCenter
                  text: diatonicBtn.modelData.degreeLabel
                }

                Label {
                  Layout.fillWidth: true
                  color: diatonicBtn.checked ? root.palette.highlightedText : root.palette.text
                  font.bold: true
                  font.pixelSize: 12
                  horizontalAlignment: Text.AlignHCenter
                  text: diatonicBtn.modelData.display
                }
              }

              onClicked: {
                root.tempRootNote = modelData.rootNote;
                root.tempChordType = modelData.chordType;
                root.tempChordAccent = MusicalScale.ChordAccent.None;
                root.tempHasBass = false;
                root.tempInversion = 0;
                root.auditionCustom();
              }
            }
          }
        }
      }
    }
  }

  onAboutToShow: {
    if (!_editingPad)
      resetToDefaults();
  }
  onAccepted: {
    root.chordSelected(root.tempRootNote, root.tempChordType, root.tempChordAccent, root.tempHasBass ? root.tempBassNote : -1, root.tempInversion);
  }
  onClosed: {
    root._editingPad = false;
  }
  onCurrentScaleChanged: root.updateScaleDependants()

  Connections {
    function onRootKeyChanged() {
      root.updateScaleDependants();
    }

    function onScaleTypeChanged() {
      root.updateScaleDependants();
    }

    target: root.currentScale ? root.currentScale.scale : null
  }
}
