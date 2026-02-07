// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle
import Zrythm
import Qt.labs.synchronizer

Dialog {
  id: root

  property QFutureQmlWrapper exportFuture
  property string metadataArtist: "Artist"
  property string metadataGenre: "Genre"
  property string metadataTitle: "Title"
  readonly property Project project: projectUiState.project
  required property ProjectUiState projectUiState

  function startExport() {
    exportProgressDialog.resetValues();
    exportProgressDialog.open();
    root.exportFuture = ProjectExporter.exportAudio(root.project, projectUiState.title);
  }

  implicitHeight: 500
  implicitWidth: 600
  modal: true
  popupType: Popup.Window
  title: qsTr("Export As...")

  // Dialog buttons
  footer: DialogButtonBox {
    Layout.fillWidth: true
    standardButtons: Dialog.Cancel

    Button {
      DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
      highlighted: true
      text: qsTr("Export")
    }
  }

  onApplied: {
    // Start export process
    startExport();
  }

  // Progress dialog for export operation
  ProgressDialogWithFuture {
    id: exportProgressDialog

    future: root.exportFuture
    labelText: qsTr("Exporting audio...")
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // Header with tab buttons
    TabBar {
      id: headerTab

      Layout.fillWidth: true
      Layout.margins: 16

      TabButton {
        text: qsTr("Audio")
      }

      TabButton {
        text: qsTr("MIDI")
      }
    }

    // Content stack
    StackLayout {
      id: stackLayout

      Layout.fillHeight: true
      Layout.fillWidth: true
      currentIndex: headerTab.currentIndex

      // Audio Export Page
      ScrollView {
        id: audioExportPage

        clip: true
        contentWidth: availableWidth

        ColumnLayout {
          spacing: 16
          width: stackLayout.width

          // First preferences page - Metadata and Options
          ZrythmPreferencesPage {
            Layout.fillWidth: true
            title: qsTr("Export Audio")

            ZrythmActionRow {
              subtitle: qsTr("Track title")
              title: qsTr("Title")

              TextField {
                id: audioTitleField

                Layout.fillWidth: true
                placeholderText: qsTr("Enter title...")
                text: root.metadataTitle

                Synchronizer on text {
                  sourceObject: root
                  sourceProperty: "metadataTitle"
                }
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Artist name")
              title: qsTr("Artist")

              TextField {
                id: audioArtistField

                Layout.fillWidth: true
                placeholderText: qsTr("Enter artist...")
                text: root.metadataArtist

                Synchronizer on text {
                  sourceObject: root
                  sourceProperty: "metadataArtist"
                }
              }
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Music genre")
            title: qsTr("Genre")

            TextField {
              id: audioGenreField

              Layout.fillWidth: true
              placeholderText: qsTr("Enter genre...")
              text: root.metadataGenre

              Synchronizer on text {
                sourceObject: root
                sourceProperty: "metadataGenre"
              }
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Audio file format")
            title: qsTr("Format")

            ComboBox {
              Layout.fillWidth: true
              model: ["WAV", "FLAC", "OGG Vorbis", "MP3"]
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Audio bit depth")
            title: qsTr("Bit Depth")

            ComboBox {
              Layout.fillWidth: true
              model: ["16", "24", "32"]
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Apply dithering")
            title: qsTr("Dither")

            Switch {
              Layout.alignment: Qt.AlignRight
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Pattern for generated filenames")
            title: qsTr("Filename Pattern")

            ComboBox {
              Layout.fillWidth: true
              model: ["{title}", "{artist} - {title}", "{track} - {title}", "Custom..."]
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Whether to export the selected tracks as a single mixdown file or each track in its own file.")
            title: qsTr("Mixdown or Stems")

            ComboBox {
              Layout.fillWidth: true
              model: [qsTr("Mixdown"), qsTr("Stems")]
            }
          }
        }

        // Second preferences page - Selection and Output
        ZrythmPreferencesPage {
          Layout.fillWidth: true
          title: qsTr("Export Audio")

          ZrythmActionRow {
            subtitle: qsTr("Only events inside this time range will be exported.")
            title: qsTr("Time Range")

            ComboBox {
              Layout.fillWidth: true
              model: [qsTr("Song Start"), qsTr("Loop"), qsTr("Custom")]
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Set custom start and end positions")
            title: qsTr("Custom Time Range")

            RowLayout {
              Layout.fillWidth: true
              spacing: 8

              Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: 120
                border.color: palette.mid
                color: palette.base
                radius: 4

                Text {
                  anchors.centerIn: parent
                  text: "0:0:0"
                }
              }

              Label {
                text: "~"
              }

              Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: 120
                border.color: palette.mid
                color: palette.base
                radius: 4

                Text {
                  anchors.centerIn: parent
                  text: "4:0:0"
                }
              }
            }
          }

          // Track Selection Expander
          ExpanderBox {
            Layout.fillWidth: true
            expanded: true
            title: qsTr("Track Selection")

            ScrollView {
              Layout.fillWidth: true
              Layout.preferredHeight: 200
              clip: true

              ListView {
                id: audioTracksView

                model: 10 // Placeholder model

                delegate: Rectangle {
                  id: audioTrackRect

                  required property int index

                  border.color: palette.mid
                  color: palette.base
                  height: 32
                  radius: 4
                  width: audioTracksView.width

                  Text {
                    anchors.left: parent.left
                    anchors.margins: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Track " + (audioTrackRect.index + 1)
                  }

                  CheckBox {
                    anchors.margins: 8
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    checked: true
                  }
                }
              }
            }
          }

          ZrythmActionRow {
            subtitle: qsTr("Export destination and file info")
            title: qsTr("Output")

            Label {
              Layout.fillWidth: true
              font.italic: true
              text: qsTr("Output will be saved to: /path/to/export/file.wav")
            }
          }
        }
      }

      // MIDI Export Page
      ScrollView {
        id: midiExportPage

        clip: true
        contentWidth: availableWidth

        ColumnLayout {
          spacing: 16
          width: stackLayout.width

          // First preferences page - Metadata and Options
          ZrythmPreferencesPage {
            Layout.fillWidth: true
            title: qsTr("Export MIDI")

            ZrythmActionRow {
              subtitle: qsTr("Track title")
              title: qsTr("Title")

              TextField {
                id: midiTitleField

                Layout.fillWidth: true
                placeholderText: qsTr("Enter title...")
                text: root.metadataTitle

                Synchronizer on text {
                  sourceObject: root
                  sourceProperty: "metadataTitle"
                }
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Artist name")
              title: qsTr("Artist")

              TextField {
                id: midiArtistField

                Layout.fillWidth: true
                placeholderText: qsTr("Enter artist...")
                text: root.metadataArtist

                Synchronizer on text {
                  sourceObject: root
                  sourceProperty: "metadataArtist"
                }
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Music genre")
              title: qsTr("Genre")

              TextField {
                id: midiGenreField

                Layout.fillWidth: true
                placeholderText: qsTr("Enter genre...")
                text: root.metadataGenre

                Synchronizer on text {
                  sourceObject: root
                  sourceProperty: "metadataGenre"
                }
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("MIDI file format")
              title: qsTr("Format")

              ComboBox {
                Layout.fillWidth: true
                model: ["Type 0", "Type 1"]
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Export MIDI lanes as separate tracks")
              title: qsTr("Export Lanes as Tracks")

              Switch {
                Layout.alignment: Qt.AlignRight
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Pattern for generated filenames")
              title: qsTr("Filename Pattern")

              ComboBox {
                Layout.fillWidth: true
                model: ["{title}", "{artist} - {title}", "{track} - {title}", "Custom..."]
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Whether to export the selected tracks as a single mixdown file or each track in its own file.")
              title: qsTr("Mixdown or Stems")

              ComboBox {
                Layout.fillWidth: true
                model: [qsTr("Mixdown"), qsTr("Stems")]
              }
            }
          }

          // Second preferences page - Selection and Output
          ZrythmPreferencesPage {
            Layout.fillWidth: true
            title: qsTr("Export MIDI")

            ZrythmActionRow {
              subtitle: qsTr("Only events inside this time range will be exported.")
              title: qsTr("Time Range")

              ComboBox {
                Layout.fillWidth: true
                model: [qsTr("Song Start"), qsTr("Loop"), qsTr("Custom")]
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Set custom start and end positions")
              title: qsTr("Custom Time Range")

              RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                  Layout.preferredHeight: 32
                  Layout.preferredWidth: 120
                  border.color: palette.mid
                  color: palette.base
                  radius: 4

                  Text {
                    anchors.centerIn: parent
                    text: "0:0:0"
                  }
                }

                Label {
                  text: "~"
                }

                Rectangle {
                  Layout.preferredHeight: 32
                  Layout.preferredWidth: 120
                  border.color: palette.mid
                  color: palette.base
                  radius: 4

                  Text {
                    anchors.centerIn: parent
                    text: "4:0:0"
                  }
                }
              }
            }

            // Track Selection Expander
            ExpanderBox {
              Layout.fillWidth: true
              expanded: true
              title: qsTr("Track Selection")

              ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                clip: true

                ListView {
                  id: midiTracksView

                  model: 5 // Placeholder model

                  delegate: Rectangle {
                    id: midiTrackRect

                    required property int index

                    border.color: palette.mid
                    color: palette.base
                    height: 32
                    radius: 4
                    width: midiTracksView.width

                    Text {
                      anchors.left: parent.left
                      anchors.margins: 8
                      anchors.verticalCenter: parent.verticalCenter
                      text: "MIDI Track " + (midiTrackRect.index + 1)
                    }

                    CheckBox {
                      anchors.margins: 8
                      anchors.right: parent.right
                      anchors.verticalCenter: parent.verticalCenter
                      checked: true
                    }
                  }
                }
              }
            }

            ZrythmActionRow {
              subtitle: qsTr("Export destination and file info")
              title: qsTr("Output")

              Label {
                Layout.fillWidth: true
                font.italic: true
                text: qsTr("Output will be saved to: /path/to/export/file.mid")
              }
            }
          }
        }
      }
    }
  }
}
