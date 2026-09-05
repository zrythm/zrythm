// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Zrythm

ColumnLayout {
  id: root

  required property StackView stackView
  property string title: qsTr("Third-Party Licenses")

  function openComponent(comp) {
    stackView.push(detailPage, {
      componentCopyright: comp.copyright,
      componentHomepage: comp.homepage,
      componentLicense: comp.license,
      componentLicenseFiles: comp.licenseFiles,
      componentName: comp.name,
      componentVersion: comp.version,
      componentVia: comp.via
    });
  }

  spacing: 4

  Component.onCompleted: {
    const jsonText = QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions.json");
    const data = JSON.parse(jsonText);
    for (const comp of data.components) {
      // ListModel turns array roles into nested sub-models (not iterable in
      // JS) and drops null members, so every role is stored as a string; the
      // full component travels as JSON for the detail page.
      componentModel.append({
        "name": comp.name ?? "",
        "subtitle": (((comp.via && comp.via !== "unknown") ? qsTr("via %1 · ").arg(comp.via) : "") + (comp.license ?? "")),
        "title": comp.name + (comp.version ? " " + comp.version : ""),
        "json": JSON.stringify(comp)
      });
    }
  }

  ListModel {
    id: componentModel
  }

  ZrythmSearchField {
    id: searchField

    Layout.fillWidth: true
    objectName: "searchField"
    placeholderText: qsTr("Search components…")
  }

  SortFilterProxyModel {
    id: filteredModel

    model: componentModel

    filters: [
      FunctionFilter {
        id: searchFilter

        property string searchText: searchField.text

        function filter(data: LicensesRoleData): bool {
          return data.name.toLowerCase().includes(searchText.toLowerCase());
        }

        onSearchTextChanged: invalidate()
      }
    ]
    sorters: [
      FunctionSorter {
        function sort(lhs: SortRoleData, rhs: SortRoleData): int {
          const a = lhs.name.toLowerCase();
          const b = rhs.name.toLowerCase();
          return a < b ? -1 : (a > b ? 1 : 0);
        }
      }
    ]
  }

  ListView {
    id: listView

    Layout.fillHeight: true
    Layout.fillWidth: true
    clip: true
    model: filteredModel
    objectName: "licensesListView"

    ScrollBar.vertical: ScrollBar {
      policy: ScrollBar.AlwaysOn
    }
    delegate: ItemDelegate {
      id: delegateItem

      required property int index
      required property string json
      required property string name
      required property string subtitle
      required property string title

      width: listView.width

      contentItem: ColumnLayout {
        spacing: 0

        Label {
          font.bold: true
          text: delegateItem.title
        }

        Label {
          font.pointSize: 8
          text: delegateItem.subtitle
        }
      }

      onClicked: root.openComponent(JSON.parse(json))
    }
  }

  Button {
    Layout.alignment: Qt.AlignHCenter
    text: qsTr("Back")

    onClicked: root.stackView.pop()
  }

  Component {
    id: detailPage

    ColumnLayout {
      id: detailRoot

      // Defaults are required: StackView applies push properties after the
      // first binding evaluation, and a binding that throws (e.g. iterating
      // an undefined list) is dropped permanently.
      property string componentCopyright: ""
      property string componentHomepage: ""
      property string componentLicense: ""
      property var componentLicenseFiles: []
      property string componentName: ""
      property var componentVersion: null
      property var componentVia: null
      property string title: componentName

      spacing: 4

      Label {
        Layout.fillWidth: true
        text: {
          let parts = [];
          if (detailRoot.componentVersion)
            parts.push(qsTr("Version %1").arg(detailRoot.componentVersion));
          if (detailRoot.componentVia && detailRoot.componentVia !== "unknown")
            parts.push(qsTr("Required by %1").arg(detailRoot.componentVia));
          if (detailRoot.componentCopyright)
            parts.push("© " + detailRoot.componentCopyright);
          if (detailRoot.componentHomepage)
            parts.push(detailRoot.componentHomepage);
          parts.push(detailRoot.componentLicense);
          return parts.join("\n");
        }
        wrapMode: Text.Wrap
      }

      ScrollView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        contentWidth: availableWidth

        ScrollBar.vertical: ScrollBar {
          policy: ScrollBar.AlwaysOn
        }

        TextArea {
          id: licenseTextArea

          font.family: "Monospace"
          font.pointSize: 9
          objectName: "licenseTextArea"
          persistentSelection: true
          readOnly: true
          selectByMouse: true
          text: {
            let text = "";
            for (const file of detailRoot.componentLicenseFiles) {
              if (text !== "")
                text += "\n\n" + "=".repeat(60) + "\n\n";
              text += QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/" + file);
            }
            return text !== "" ? text : qsTr("License text not bundled; see the component homepage.");
          }
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

        onClicked: root.stackView.pop()
      }
    }
  }

  component LicensesRoleData: QtObject {
    property string name
  }
  component SortRoleData: QtObject {
    property string name
  }
}
