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
          text: qsTr("Trademark Policy")

          onClicked: stackView.push(licensePage, {
            licenseTitle: "Zrythm Trademark Policy",
            licenseText: QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/TRADEMARKS.md")
          })
        }

        Button {
          flat: true
          text: qsTr("Third Party Notices")

          onClicked: stackView.push(licensePage, {
            licenseTitle: "Third Party Notices",
            licenseText: QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions_sbom.txt")
          })
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
          persistentSelection: true
          readOnly: true
          selectByMouse: true
          text: licensePageColLayout.licenseText
          wrapMode: Text.Wrap

          Action {
            id: copyAction

            enabled: licenseTextArea.selectedText
            shortcut: StandardKey.Copy
            text: qsTr("&Copy")

            onTriggered: licenseTextArea.copy()
          }

          Action {
            id: selectAllAction

            enabled: true
            shortcut: StandardKey.SelectAll
            text: qsTr("Select All")

            onTriggered: licenseTextArea.selectAll()
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
