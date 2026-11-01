// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import Qt.labs.synchronizer
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  title: qsTr("Advanced")

  ActionRowGroup {
    title: qsTr("Diagnostics")
    subtitle: qsTr("Instrumentation for debugging the application")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Show playback cache status while the engine is running")
      title: qsTr("Show Cache Activity")

      Switch {
        Synchronizer on checked {
          sourceObject: root.appSettings
          sourceProperty: "showCacheActivity"
        }
      }
    }
  }

  ActionRowGroup {
    title: qsTr("Maintenance")
    subtitle: qsTr("Restoring default application state")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      subtitle: qsTr("Restore every preference to its default value")
      title: qsTr("Reset To Factory Settings")

      DestructiveButton {
        objectName: "resetPreferencesButton"
        text: qsTr("Reset All Preferences…")

        onClicked: resetDialog.open()
      }
    }
  }

  Dialog {
    id: resetDialog

    modal: true
    objectName: "resetPreferencesDialog"
    popupType: Popup.Window
    title: qsTr("Reset Preferences?")
    width: 360

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
          text: qsTr("Reset Preferences?")
        }
      }

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.normalTextFont
        text: qsTr("This resets the preferences shown in this dialog. Projects and recent project entries are not changed.")
        wrapMode: Text.WordWrap
      }
    }

    footer: DialogButtonBox {
      Button {
        objectName: "resetPreferencesCancelButton"
        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        text: qsTr("Cancel")

        onClicked: resetDialog.close()
      }

      DestructiveButton {
        objectName: "resetPreferencesConfirmButton"
        DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        text: qsTr("Reset")

        onClicked: {
          if (root.appSettings !== null)
            root.appSettings.reset_preference_settings();
          const translationManager = GlobalState.application
            ? GlobalState.application.translationManager
            : null;
          if (translationManager !== null)
            translationManager.loadTranslation("");
          resetDialog.close();
        }
      }
    }
  }
}
