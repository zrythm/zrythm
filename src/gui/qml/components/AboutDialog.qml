// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import "../config.js" as Config

Dialog {
  id: root

  implicitHeight: 500
  implicitWidth: 700
  modal: true
  popupType: Popup.Window
  standardButtons: Dialog.Ok
  title: stackView.currentItem ? stackView.currentItem.title : qsTr("About Zrythm")
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
              Layout.preferredWidth: 200
              font.bold: true
              text: qsTr("License Notice")
            }

            Label {
              Layout.fillWidth: true
              font.bold: true
              text: qsTr("Legal Text")
            }
          }

          // License entries
          Repeater {
            delegate: RowLayout {
              id: thirdPartyRow

              required property string licenseIdentifier
              required property string licenseNotice
              required property string licenseText
              required property string name

              Layout.fillWidth: true
              spacing: 10

              Label {
                Layout.preferredWidth: 200
                text: thirdPartyRow.name
                // wrapMode: Text.Wrap
              }

              Button {
                Layout.preferredWidth: 200
                flat: true
                text: qsTr("View License Notice")

                onClicked: {
                  stackView.push(licensePage, {
                    licenseTitle: thirdPartyRow.name + " - License Notice",
                    licenseText: thirdPartyRow.licenseNotice
                  });
                }
              }

              // License button
              Button {
                readonly property string licenseName: thirdPartyRow.licenseIdentifier

                Layout.fillWidth: true
                flat: true
                text: licenseName

                onClicked: {
                  stackView.push(licensePage, {
                    licenseTitle: thirdPartyRow.name + " - " + licenseName,
                    licenseText: thirdPartyRow.licenseText
                  });
                }
              }
            }
            model: ThirdPartyLicensesModel {
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
