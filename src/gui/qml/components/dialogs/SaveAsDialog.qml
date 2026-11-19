// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Dialog {
  id: root

  required property string initialName
  property url initialLocation
  required property ProjectManager projectManager

  // Classification of the directory the current name and location
  // resolve to; refreshed on every name and location change
  property int targetState: ProjectManager.Unknown

  signal saveAccepted(url parentFolder, string name, bool overwrite)

  readonly property string targetPath: {
    const parentPath = QmlUtils.toPathString(locationPicker.selectedUrl);
    if (parentPath.length === 0)
      return "";
    return parentPath + "/" + nameField.text;
  }

  function attemptAccept(): void {
    switch (root.targetState) {
      case ProjectManager.NonExistent:
      case ProjectManager.Empty:
        root.accept();
        break;
      case ProjectManager.ContainsProjectFile:
        replaceConfirmDialog.open();
        break;
      default:
        break;
    }
  }

  function refreshTargetState(): void {
    if (QmlUtils.toPathString(locationPicker.selectedUrl).length === 0) {
      root.targetState = ProjectManager.Unknown;
      statusLabel.text = qsTr("Choose a location for the project.");
      return;
    }
    if (nameField.text.length === 0) {
      root.targetState = ProjectManager.InvalidName;
      statusLabel.text = qsTr("Enter a name for the project.");
      return;
    }
    if (!projectManager.isValidProjectName(nameField.text)) {
      root.targetState = ProjectManager.InvalidName;
      statusLabel.text = qsTr("This name cannot be used for a project folder.");
      return;
    }

    root.targetState = projectManager.classifySaveTarget(
      locationPicker.selectedUrl, nameField.text);
    switch (root.targetState) {
      case ProjectManager.OtherContent:
        statusLabel.text = qsTr("A folder with this name already exists and holds other files. Choose another name or location.");
        break;
      case ProjectManager.Unknown:
        statusLabel.text = qsTr("Zrythm could not check this location. Choose another location.");
        break;
      case ProjectManager.ContainsProjectFile:
        statusLabel.text = qsTr("A project already exists here. Replacing it overwrites the project file; its other files stay on disk.");
        break;
      default:
        statusLabel.text = "";
        break;
    }
  }

  modal: true
  popupType: Popup.Window
  title: qsTr("Save Project As")
  width: 520

  onAboutToShow: {
    nameField.text = root.initialName;
    root.refreshTargetState();
    nameField.selectAll();
    nameField.forceActiveFocus();
  }

  onAccepted: {
    root.saveAccepted(
      locationPicker.selectedUrl,
      nameField.text,
      root.targetState === ProjectManager.ContainsProjectFile);
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: ZrythmTheme.buttonPadding

    ActionRowGroup {
      Layout.fillWidth: true
      title: qsTr("Destination")

      ActionRow {
        subtitle: qsTr("Also used as the folder name")
        title: qsTr("Name")

        TextField {
          id: nameField

          Layout.preferredWidth: 240

          onAccepted: root.attemptAccept()
          onTextChanged: root.refreshTargetState()
        }
      }

      ActionRow {
        subtitle: qsTr("Where the project folder will be created")
        title: qsTr("Location")

        ZrythmFilePicker {
          id: locationPicker

          Layout.preferredWidth: 300
          initialPath: QmlUtils.toPathString(root.initialLocation)
        }
      }
    }

    Label {
      Layout.fillWidth: true
      color: palette.text
      elide: Text.ElideMiddle
      font: ZrythmTheme.smallTextFont
      opacity: 0.55
      text: root.targetPath
      visible: root.targetPath !== ""
    }

    Label {
      id: statusLabel

      Layout.fillWidth: true
      color: ZrythmTheme.errorTextColor
      font: ZrythmTheme.smallTextFont
      visible: text !== ""
      wrapMode: Text.WordWrap
    }
  }

  Connections {
    function onSelectedUrlChanged() {
      root.refreshTargetState();
    }

    target: locationPicker
  }

  Dialog {
    id: replaceConfirmDialog

    // The confirmation renders inside the save dialog's window
    closePolicy: Popup.CloseOnEscape
    modal: true
    parent: root.contentItem
    title: qsTr("Replace Project?")
    width: 400

    ColumnLayout {
      anchors.fill: parent
      spacing: ZrythmTheme.buttonPadding

      RowLayout {
        Layout.fillWidth: true
        spacing: ZrythmTheme.buttonPadding

        IconImage {
          Layout.preferredHeight: 16
          Layout.preferredWidth: 16
          color: ZrythmTheme.warningColor
          source: ResourceManager.getIconUrl(
            "gnome-icon-library", "warning-outline-symbolic.svg")
          sourceSize.height: 16
          sourceSize.width: 16
        }

        Label {
          Layout.fillWidth: true
          color: palette.text
          font: ZrythmTheme.semiBoldTextFont
          text: qsTr("Replace Project?")
        }
      }

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.normalTextFont
        text: qsTr("The project file at %1 will be overwritten. The other files in this folder stay on disk.").arg(
          root.targetPath)
        wrapMode: Text.WordWrap
      }
    }

    footer: DialogButtonBox {
      Button {
        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        text: qsTr("Cancel")

        onClicked: replaceConfirmDialog.close()
      }

      DestructiveButton {
        DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        text: qsTr("Replace")

        onClicked: {
          replaceConfirmDialog.close();
          root.accept();
        }
      }
    }
  }

  footer: DialogButtonBox {
    Button {
      DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
      text: qsTr("Cancel")
    }

    Button {
      highlighted: true
      enabled: root.targetState === ProjectManager.NonExistent
        || root.targetState === ProjectManager.Empty
        || root.targetState === ProjectManager.ContainsProjectFile
      text: root.targetState === ProjectManager.ContainsProjectFile
        ? qsTr("Replace")
        : qsTr("Save")
      // Replace carries the destructive error accent; every other
      // state uses the accent inherited from the dialog
      palette.accent: root.targetState === ProjectManager.ContainsProjectFile
        ? ZrythmTheme.errorColor
        : root.palette.accent

      onClicked: root.attemptAccept()
    }
  }
}
