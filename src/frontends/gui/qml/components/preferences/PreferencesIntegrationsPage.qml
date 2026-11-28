// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import Zrythm
import ZrythmMcp
import ZrythmStyle

ZrythmPreferencesPage {
  id: root

  property AppSettings appSettings: null
  // The grants a token can have, in the order the picker lists them.
  readonly property var grants: [
    {
      "description": qsTr("Can read the project. Cannot change anything."),
      "identity": "query",
      "title": qsTr("Read Only")
    },
    {
      "description": qsTr("Can read the project and control playback, live parameters and saving. Cannot edit the project's content."),
      "identity": "control",
      "title": qsTr("Read & Control")
    },
    {
      "description": qsTr("Can read, control and edit the project. Every edit it makes can be undone."),
      "identity": "full",
      "title": qsTr("Full Access")
    }
  ]
  property McpEndpoint mcpEndpoint: null

  // The subtitle of the enable row. While the endpoint starts, listens
  // or has failed it names that state; otherwise it says what the
  // settings allow.
  readonly property string serverSubtitle: {
    const endpoint = root.mcpEndpoint;
    if (endpoint !== null) {
      if (endpoint.state === McpEndpoint.Starting)
        return qsTr("Starting…");
      if (endpoint.state === McpEndpoint.Failed)
        return qsTr("The server is not running.");
    }
    if (!controller.canEnable)
      return qsTr("Add an access token to enable the server.");
    return qsTr("Clients connect to %1").arg(controller.endpointUrl);
  }
  title: qsTr("Integrations")

  McpSettingsController {
    id: controller

    appSettings: root.appSettings
    objectName: "mcpSettingsController"
  }

  Connections {
    target: controller

    function onPortChanged(): void {
      portField.text = controller.port;
    }
  }

  // Shown when the process environment takes the place of the settings.
  RowLayout {
    Layout.fillWidth: true
    objectName: "mcpOverrideNotice"
    spacing: ZrythmTheme.buttonPadding
    visible: controller.settingsOverridden

    IconImage {
      Layout.preferredHeight: 16
      Layout.preferredWidth: 16
      color: ZrythmTheme.warningColor
      source: ResourceManager.getIconUrl("gnome-icon-library", "warning-outline-symbolic.svg")
      sourceSize.height: 16
      sourceSize.width: 16
    }

    Label {
      id: overrideLabel

      Layout.fillWidth: true
      color: palette.text
      objectName: "mcpOverrideLabel"
      text: controller.overrideMalformed ? qsTr("ZRYTHM_MCP_TOKEN is set but not valid, or ZRYTHM_MCP_PORT is not a port number. The MCP server stays off in this session, and the controls on this page have no effect.") : qsTr("The environment variable ZRYTHM_MCP_TOKEN is set. The MCP server uses that token in this session, and the controls on this page have no effect.")
      wrapMode: Text.WordWrap
    }
  }

  ActionRowGroup {
    enabled: !controller.settingsOverridden
    title: qsTr("MCP Server")
    subtitle: qsTr("Let AI assistants and other MCP clients connect to Zrythm on this computer")

    ActionRow {
      id: enabledRow

      hoverBaseColor: ZrythmTheme.cardColor
      objectName: "mcpEnabledRow"
      subtitle: root.serverSubtitle
      title: qsTr("Enable MCP Server")

      Switch {
        id: enableSwitch

        checked: controller.enabled
        enabled: controller.canEnable
        objectName: "mcpEnabledSwitch"

        onToggled: {
          if (!controller.setEnabled(enableSwitch.checked))
            enableSwitch.checked = Qt.binding(() => controller.enabled);
        }
      }
    }

    // Shown while the endpoint reports why it could not start.
    RowLayout {
      Layout.fillWidth: true
      Layout.leftMargin: enabledRow.leftPadding
      Layout.rightMargin: enabledRow.rightPadding
      objectName: "mcpFailureNotice"
      spacing: ZrythmTheme.buttonPadding
      visible: root.mcpEndpoint !== null && root.mcpEndpoint.state === McpEndpoint.Failed

      IconImage {
        Layout.alignment: Qt.AlignTop
        Layout.preferredHeight: 16
        Layout.preferredWidth: 16
        color: ZrythmTheme.errorTextColor
        source: ResourceManager.getIconUrl("gnome-icon-library", "warning-outline-symbolic.svg")
        sourceSize.height: 16
        sourceSize.width: 16
      }

      Label {
        id: failureLabel

        Layout.fillWidth: true
        color: ZrythmTheme.errorTextColor
        objectName: "mcpFailureLabel"
        text: root.mcpEndpoint?.errorText ?? ""
        wrapMode: Text.WordWrap
      }
    }

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      objectName: "mcpPortRow"
      subtitle: qsTr("The port clients connect to")
      title: qsTr("MCP Port")

      TextField {
        id: portField

        Layout.preferredWidth: 96
        objectName: "mcpPortField"
        text: controller.port
        validator: RegularExpressionValidator {
          regularExpression: /[0-9]{0,5}/
        }

        onAccepted: root.applyPort()

        onActiveFocusChanged: {
          if (!portField.activeFocus)
            root.applyPort();
        }

        onTextEdited: portErrorLabel.visible = false
      }

      Label {
        id: portErrorLabel

        color: ZrythmTheme.errorTextColor
        font: ZrythmTheme.smallTextFont
        objectName: "mcpPortError"
        text: qsTr("Enter a port number from 1 to 65535.")
        visible: false
      }
    }
  }

  ActionRowGroup {
    enabled: !controller.settingsOverridden
    title: qsTr("MCP Access Tokens")
    subtitle: qsTr("Each token lets one client connect. Its grant decides what the client can do")

    ActionRow {
      hoverBaseColor: ZrythmTheme.cardColor
      objectName: "mcpTokensRow"
      title: qsTr("Access Tokens")

      Button {
        objectName: "addTokenButton"
        text: qsTr("Add Token…")

        onClicked: addDialog.open()
      }
    }

    Repeater {
      model: controller.tokens

      delegate: ActionRow {
        id: tokenRow

        required property int index
        required property var modelData

        hoverBaseColor: ZrythmTheme.cardColor
        objectName: "tokenRow"
        searchable: false
        subtitle: root.grantTitle(tokenRow.modelData.grant)
        title: tokenRow.modelData.name

        DestructiveButton {
          objectName: "removeTokenButton"
          text: qsTr("Remove")

          onClicked: controller.removeToken(tokenRow.index)
        }
      }
    }

    Label {
      color: palette.text
      font: ZrythmTheme.smallTextFont
      objectName: "noTokensLabel"
      opacity: 0.55
      padding: 8
      text: qsTr("No access tokens.")
      visible: controller.tokens.length === 0
    }
  }

  Dialog {
    id: addDialog

    closePolicy: Popup.CloseOnEscape
    modal: true
    objectName: "addTokenDialog"
    parent: root
    title: qsTr("Add Access Token")
    width: 420

    onAboutToShow: {
      nameField.text = "";
      grantCombo.currentIndex = 0;
    }
    onAccepted: root.addToken()

    ColumnLayout {
      anchors.fill: parent
      spacing: ZrythmTheme.buttonPadding

      GridLayout {
        Layout.fillWidth: true
        columnSpacing: ZrythmTheme.buttonPadding
        columns: 2
        rowSpacing: ZrythmTheme.buttonPadding

        Label {
          text: qsTr("Name")
        }

        TextField {
          id: nameField

          Layout.fillWidth: true
          objectName: "tokenNameField"
          placeholderText: qsTr("Enter a name")
        }

        Label {
          text: qsTr("Grant")
        }

        ComboBox {
          id: grantCombo

          Layout.fillWidth: true
          model: root.grants
          objectName: "tokenGrantCombo"
          textRole: "title"
        }
      }

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        objectName: "grantDescription"
        text: root.grants[grantCombo.currentIndex]?.description ?? ""
        wrapMode: Text.WordWrap
      }

      Label {
        id: nameErrorLabel

        Layout.fillWidth: true
        color: ZrythmTheme.errorTextColor
        font: ZrythmTheme.smallTextFont
        objectName: "tokenNameError"
        text: controller.nameError(nameField.text)
        visible: text !== ""
        wrapMode: Text.WordWrap
      }
    }

    footer: DialogButtonBox {
      defaultButton: addConfirmButton

      Button {
        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        objectName: "addTokenCancelButton"
        text: qsTr("Cancel")
      }

      Button {
        id: addConfirmButton

        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        enabled: nameErrorLabel.text === ""
        highlighted: true
        objectName: "addTokenConfirmButton"
        text: qsTr("Add Token")
      }
    }
  }

  Dialog {
    id: secretDialog

    // The secret is kept only while this dialog shows it.
    property string secret: ""
    property string tokenName: ""

    closePolicy: Popup.CloseOnEscape
    modal: true
    objectName: "tokenSecretDialog"
    parent: root
    title: qsTr("Access Token Created")
    width: 420

    onClosed: {
      secretDialog.secret = "";
      secretDialog.tokenName = "";
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: ZrythmTheme.buttonPadding

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        text: qsTr("Server Address")
      }

      Label {
        Layout.fillWidth: true
        objectName: "secretServerAddress"
        text: controller.endpointUrl
        wrapMode: Text.WrapAnywhere
      }

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        text: qsTr("Secret for “%1”").arg(secretDialog.tokenName)
        wrapMode: Text.WordWrap
      }

      TextField {
        Layout.fillWidth: true
        objectName: "tokenSecretField"
        readOnly: true
        selectByMouse: true
        text: secretDialog.secret
      }

      Label {
        Layout.fillWidth: true
        color: palette.text
        font: ZrythmTheme.smallTextFont
        opacity: 0.55
        text: qsTr("Zrythm shows this secret only once and stores only its hash. If you lose it, remove the token and add a new one.")
        wrapMode: Text.WordWrap
      }
    }

    footer: DialogButtonBox {
      defaultButton: doneButton

      Button {
        DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
        objectName: "copySecretButton"
        text: qsTr("Copy")

        onClicked: QmlUtils.copyToClipboard(secretDialog.secret)
      }

      Button {
        id: doneButton

        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        highlighted: true
        objectName: "secretDoneButton"
        text: qsTr("Done")
      }
    }
  }

  function grantTitle(identity: string): string {
    const grant = root.grants.find((grant) => grant.identity === identity);
    return grant ? grant.title : identity;
  }

  // Sends the port in the field to the settings. A refused port shows the
  // message and returns the field to the stored port.
  function applyPort(): void {
    const value = Number(portField.text);
    if (!Number.isInteger(value) || !controller.setPort(value)) {
      portErrorLabel.visible = true;
      portField.text = controller.port;
      return;
    }
    portErrorLabel.visible = false;
    portField.text = controller.port;
  }

  // Adds the token named in the add dialog and shows its secret, which
  // only the secret dialog displays.
  function addToken(): void {
    const grant = root.grants[grantCombo.currentIndex];
    const secret = controller.addToken(nameField.text, grant.identity);
    if (secret === "")
      return;
    secretDialog.tokenName = nameField.text;
    secretDialog.secret = secret;
    secretDialog.open();
  }
}
