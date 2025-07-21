// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
  id: root

  readonly property real spacing: 10

  height: 720
  title: "Demo View"
  visible: true
  width: 1280

  header: ToolBar {
    id: toolBar

    RowLayout {
      Repeater {
        // put widgets here

        model: [true, false]

        RowLayout {
          // put things in here

          id: enabledRepeater

          property bool enabledBoolData: modelData

          Repeater {
            model: [true, false]

            Flow {
              id: checkedFlow

              property bool checkableBoolData: modelData

              ToolButton {
                checkable: checkedFlow.checkableBoolData
                enabled: enabledRepeater.enabledBoolData
                text: "Tool Button"
              }

              ToolButton {
                checkable: checkedFlow.checkableBoolData
                checked: checkedFlow.checkableBoolData
                enabled: enabledRepeater.enabledBoolData
                icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm-monochrome.svg")
                text: "Tool Button"
              }

              ToolButton {
                checkable: checkedFlow.checkableBoolData
                enabled: enabledRepeater.enabledBoolData
                icon.source: ResourceManager.getIconUrl("zrythm-dark", "zrythm-monochrome.svg")
              }
            }
          }

          ToolSeparator {
            enabled: enabledRepeater.enabledBoolData
          }

          SplitButton {
            id: undoBtn

            enabled: enabledRepeater.enabledBoolData
            iconSource: ResourceManager.getIconUrl("zrythm-dark", "edit-undo.svg")
            menuTooltipText: qsTr("Undo Multiple")
            tooltipText: qsTr("Undo")

            menuItems: Menu {
              Action {
                text: qsTr("Undo Move")
              }

              Action {
                text: qsTr("Undo Cut")
              }
            }
          }

          SplitButton {
            id: redoBtn

            enabled: enabledRepeater.enabledBoolData
            iconSource: ResourceManager.getIconUrl("zrythm-dark", "edit-redo.svg")
            menuTooltipText: qsTr("Redo Multiple")
            tooltipText: qsTr("Redo")
          }
        }
      }
    }
  }
  menuBar: MenuBar {
    Menu {
      title: qsTr("&File")

      Action {
        text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
      }

      Action {
        text: qsTr("Open")
      }
    }

    Menu {
      title: qsTr("&Edit")

      Action {
        text: qsTr("Undo")
      }
    }

    Menu {
      title: qsTr("&View")

      Menu {
        title: qsTr("Appearance")

        Action {
          text: qsTr("Switch Light/Dark Theme")

          onTriggered: {
            Style.darkMode = !Style.darkMode;
          }
        }

        Menu {
          title: qsTr("Theme Color")

          Action {
            text: qsTr("Zrythm Orange")

            onTriggered: {
              Style.primaryColor = Style.zrythmColor;
            }
          }

          Action {
            text: qsTr("Celestial Blue")

            onTriggered: {
              Style.primaryColor = Style.celestialBlueColor;
            }
          }

          Action {
            text: qsTr("Jonquil Yellow")

            onTriggered: {
              Style.primaryColor = Style.jonquilYellowColor;
            }
          }
        }
      }
    }

    Menu {
      title: qsTr("&Help")

      MenuItem {
        text: qsTr("Something")

        onTriggered: {}
      }

      Action {
        text: "Something 2"
      }

      Action {
        text: "Something 3"
      }

      Action {
        text: "Something 4"
      }

      Action {
        text: "Somet&hing 5"
      }

      Action {
        text: "Something 6"
      }
    }
  }

  Flow {
    id: outerFlow

    anchors.fill: parent

    Repeater {
      model: [true, false]

      Flow {
        // put things in here

        id: enabledRepeater

        property bool enabledBoolData: modelData

        spacing: root.spacing
        width: parent.width

        Flow {
          id: allWidgetsContainer

          padding: 12
          spacing: root.spacing
          width: parent.width

          Repeater {
            id: buttonRepeater

            model: 3

            Flow {
              spacing: root.spacing

              Button {
                checkable: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                text: "Button"

                ToolTip {
                  text: "Some long tooltip text"
                }
              }

              Button {
                checkable: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                text: "Button"
              }

              Button {
                checkable: modelData === 1
                checked: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
              }

              RoundButton {
                checkable: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                text: "Button"

                ToolTip {
                  text: "Some long tooltip text"
                }
              }

              RoundButton {
                checkable: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
                text: "Button"
              }

              RoundButton {
                checkable: modelData === 1
                checked: modelData === 1
                enabled: enabledRepeater.enabledBoolData
                highlighted: modelData === 2
                icon.source: Qt.resolvedUrl("qrc:/qt/qml/Zrythm/icons/zrythm-dark/zrythm-monochrome.svg")
              }
            }
          }

          ComboBox {
            enabled: enabledRepeater.enabledBoolData
            model: ['日本語', 'bbbb long text', 'short']
          }

          ProgressBar {
            enabled: enabledRepeater.enabledBoolData
            indeterminate: true
          }

          ProgressBar {
            enabled: enabledRepeater.enabledBoolData
            from: 0
            to: 100
            value: 50

            // Dummy animation to move the progress bar back and forth
            SequentialAnimation on value {
              loops: Animation.Infinite

              NumberAnimation {
                duration: 2000
                easing.type: Style.animationEasingType
                from: 0
                to: 100
              }

              NumberAnimation {
                duration: 2000
                easing.type: Style.animationEasingType
                from: 100
                to: 0
              }
            }
          }

          TabBar {
            TabButton {
              enabled: enabledRepeater.enabledBoolData
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "piano-roll.svg")
              text: qsTr("Editor")
            }

            TabButton {
              enabled: enabledRepeater.enabledBoolData
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "mixer.svg")
              text: qsTr("Mixer")
            }

            TabButton {
              enabled: enabledRepeater.enabledBoolData
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "encoder-knob-symbolic.svg")
              text: qsTr("Modulators")
            }

            TabButton {
              enabled: enabledRepeater.enabledBoolData
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "chord-pad.svg")
              text: "Chord Pad"
            }
          }

          TabBar {
            enabled: enabledRepeater.enabledBoolData

            TabButton {
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "track-inspector.svg")
              text: qsTr("Track Inspector")
            }

            TabButton {
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "plug.svg")
              text: qsTr("Plugin Inspector")
            }
          }

          TabBar {
            enabled: enabledRepeater.enabledBoolData

            TabButton {
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "shapes-large-symbolic.svg")

              ToolTip {
                text: qsTr("Plugin Browser")
              }
            }

            TabButton {
              icon.source: ResourceManager.getIconUrl("gnome-icon-library", "file-cabinet-symbolic.svg")

              ToolTip {
                text: qsTr("File Browser")
              }
            }

            TabButton {
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "speaker.svg")

              ToolTip {
                text: qsTr("Monitor Section")
              }
            }

            TabButton {
              icon.source: ResourceManager.getIconUrl("zrythm-dark", "minuet-chords.svg")

              ToolTip {
                text: qsTr("Chord Preset Browser")
              }
            }
          }

          TextField {
            enabled: enabledRepeater.enabledBoolData
            placeholderText: "Placeholder text"
          }

          TextField {
            enabled: enabledRepeater.enabledBoolData
            placeholderText: "Placeholder text"
            text: "Text"
          }
        }
      }
    }
  }
}
