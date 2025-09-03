// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import "../config.js" as Config
import "../licenses.js" as Licenses

Dialog {
  id: root

  implicitHeight: 500
  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Ok
  title: stackView.currentItem ? stackView.currentItem.title : qsTr("About Zrythm")
  implicitWidth: 700
  x: (parent.width - width) / 2
  y: (parent.height - height) / 2

  onAccepted: {
    // Ensure we return to main page when dialog is closed
    if (stackView.depth > 1) {
      stackView.pop(null, StackView.Immediate);
    }
  }
  onRejected: {
    // Ensure we return to main page when dialog is closed
    if (stackView.depth > 1) {
      stackView.pop(null, StackView.Immediate);
    }
  }

  StackView {
    id: stackView

    anchors.fill: parent
    initialItem: mainPage

    popEnter: Transition {
    }
    popExit: Transition {
    }
    pushEnter: Transition {
    }
    pushExit: Transition {
    }
  }

  Component {
    id: mainPage

    ColumnLayout {
      property string title: qsTr("About Zrythm")

      spacing: 8

      Image {
        Layout.alignment: Qt.AlignHCenter
        source: ResourceManager.getIconUrl("zrythm-dark", "zrythm.svg")
        sourceSize.height: 64
        sourceSize.width: 64
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        font.bold: true
        font.pointSize: 16
        text: "Zrythm"
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        text: "Version %1".arg(Config.VERSION)
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        font.pointSize: 10
        text: "© %1 %2. All rights reserved.".arg(Config.COPYRIGHT_YEARS).arg(Config.COPYRIGHT_NAME)
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        font.italic: true
        font.pointSize: 9
        text: qsTr("Licensed under the GNU AGPLv3 License.")
      }

      RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Button {
          flat: true
          text: qsTr("View License")

          onClicked: stackView.push(licensePage, {
            licenseTitle: "Zrythm License",
            licenseText: QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/LicenseRef-ZrythmLicense.txt")
          })
        }

        Button {
          flat: true
          text: qsTr("View Trademark Policy")

          onClicked: stackView.push(licensePage, {
            licenseTitle: "Zrythm Trademark Policy",
            licenseText: QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/TRADEMARKS.md")
          })
        }

        Button {
          flat: true
          text: qsTr("Third-party Licenses")

          onClicked: stackView.push(otherLicensesPage)
        }
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        font.italic: true
        font.pointSize: 8
        text: qsTr("Zrythm and the Zrythm logo are trademarks of Alexandros Theodotou.")
      }
    }
  }

  Component {
    id: licensePage

    ColumnLayout {
      id: licensePageColLayout

      property string licenseText
      property string licenseTitle
      property string title: licenseTitle

      ScrollView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        contentWidth: availableWidth

        TextArea {
          id: licenseTextArea

          font.family: "Monospace"
          font.pointSize: 9
          readOnly: true
          selectByMouse: true
          text: licensePageColLayout.licenseText
          wrapMode: Text.Wrap
        }
      }

      Button {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Back")

        onClicked: stackView.pop()
      }
    }
  }

  Component {
    id: otherLicensesPage

    ColumnLayout {
      property string title: qsTr("Third-party License Notices")

      Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 10
        font.bold: true
        font.pointSize: 14
        text: qsTr("Third-Party Library Licenses")
      }

      ScrollView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        contentWidth: availableWidth

        ColumnLayout {
          spacing: 2
          width: parent.width

          // Header row
          RowLayout {
            Layout.bottomMargin: 10
            Layout.fillWidth: true
            spacing: 10

            Label {
              Layout.preferredWidth: 200
              font.bold: true
              text: qsTr("Library")
            }

            Label {
              Layout.fillWidth: true
              font.bold: true
              text: qsTr("Copyright")
            }

            Label {
              Layout.fillWidth: true
              font.bold: true
              text: qsTr("License")
            }
          }

          // License entries
          Repeater {
            model: Licenses.ThirdPartyLicenses

            RowLayout {
              id: thirdPartyRow

              required property string copyright
              required property string license
              required property string name

              Layout.fillWidth: true
              spacing: 10

              Label {
                Layout.preferredWidth: 200
                text: thirdPartyRow.name
                // wrapMode: Text.Wrap
              }

              Label {
                Layout.fillWidth: true
                text: "© " + thirdPartyRow.copyright
                // wrapMode: Text.Wrap
              }

              // License buttons - handle "OR" expressions
              Flow {
                Layout.fillWidth: true
                spacing: 4

                Repeater {
                  model: {
                    var licenses = [];
                    var parts = thirdPartyRow.license.split(/\s+/);
                    for (var i = 0; i < parts.length; i++) {
                      var part = parts[i].trim();
                      if (part && part !== "OR") {
                        licenses.push(part);
                      }
                    }
                    return licenses;
                  }

                  Button {
                    required property string modelData

                    flat: true
                    text: modelData

                    onClicked: {
                      var licenseFile = ":/qt/qml/Zrythm/licenses/" + modelData + ".txt";
                      stackView.push(licensePage, {
                        licenseTitle: thirdPartyRow.name + " - " + modelData,
                        licenseText: QmlUtils.readTextFileContent(licenseFile)
                      });
                    }
                  }
                }
              }
            }
          }
        }
      }

      Button {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Back")

        onClicked: stackView.pop()
      }
    }
  }
}
