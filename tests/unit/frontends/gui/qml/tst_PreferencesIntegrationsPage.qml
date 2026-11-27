// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: test

  name: "PreferencesIntegrationsPage"
  height: 700
  visible: true
  when: windowShown
  width: 700

  Component {
    id: pageComponent

    PreferencesIntegrationsPage {
      appSettings: testAppSettings
      height: 600
      width: 600
    }
  }

  function init(): void {
    testAppSettings.reset_mcpEnabled();
    testAppSettings.reset_mcpPort();
    testAppSettings.reset_mcpTokens();
    testUtils.unsetEnvironmentVariable("ZRYTHM_MCP_TOKEN");
  }

  function cleanup(): void {
    testUtils.unsetEnvironmentVariable("ZRYTHM_MCP_TOKEN");
  }

  // Every item below item, depth first, in declaration order.
  function collectItems(item): array {
    const result = [item];
    for (let i = 0; i < item.children.length; ++i)
      result.push(...collectItems(item.children[i]));
    return result;
  }

  function tokenRows(page): array {
    return collectItems(page).filter((item) => item.objectName === "tokenRow");
  }

  function makePage(): PreferencesIntegrationsPage {
    const page = createTemporaryObject(pageComponent, test);
    verify(page);
    return page;
  }

  function controllerOf(page): McpSettingsController {
    const controller = findChild(page, "mcpSettingsController");
    verify(controller);
    return controller;
  }

  function test_secret_is_shown_once() {
    const page = makePage();
    const addButton = findChild(page, "addTokenButton");
    const addDialog = findChild(page, "addTokenDialog");
    const nameField = findChild(page, "tokenNameField");
    const grantCombo = findChild(page, "tokenGrantCombo");
    const confirmButton = findChild(page, "addTokenConfirmButton");
    const secretDialog = findChild(page, "tokenSecretDialog");
    const secretField = findChild(page, "tokenSecretField");
    const copyButton = findChild(page, "copySecretButton");
    const doneButton = findChild(page, "secretDoneButton");

    addButton.clicked();
    tryVerify(() => addDialog.visible);
    nameField.text = "Studio Desk";
    grantCombo.currentIndex = 1;
    compare(confirmButton.enabled, true);
    confirmButton.clicked();
    tryVerify(() => secretDialog.visible);
    const secret = secretField.text;
    compare(secret.length, 64);
    verify(/^[0-9a-f]+$/.test(secret));

    copyButton.clicked();
    compare(testUtils.clipboardText(), secret);

    doneButton.clicked();
    tryVerify(() => !secretDialog.visible);

    // Nothing on the page shows the secret any more.
    compare(secretField.text, "");
    for (const item of collectItems(page)) {
      if (typeof item.text === "string")
        verify(item.text !== secret);
      if (typeof item.title === "string")
        verify(item.title !== secret);
    }

    // The settings hold the hash only, and the row shows name and grant.
    const entries = testAppSettings.mcpTokens;
    compare(entries.length, 1);
    verify(/^Studio Desk\|control\|[0-9a-f]{64}$/.test(entries[0]));
    verify(!entries[0].includes(secret));
    const rows = tokenRows(page);
    compare(rows.length, 1);
    compare(rows[0].title, "Studio Desk");
    compare(rows[0].subtitle, "Read & Control");
  }

  function test_enable_is_refused_without_tokens() {
    const page = makePage();
    const controller = controllerOf(page);
    const enableSwitch = findChild(page, "mcpEnabledSwitch");
    verify(enableSwitch);

    compare(enableSwitch.enabled, false);
    compare(enableSwitch.checked, false);
    compare(controller.setEnabled(true), false);
    verify(!testAppSettings.mcpEnabled);
    compare(enableSwitch.checked, false);

    // With a token the switch follows the setting both ways.
    verify(controller.addToken("studio", "full") !== "");
    compare(enableSwitch.enabled, true);
    mouseClick(enableSwitch);
    verify(testAppSettings.mcpEnabled);
    compare(enableSwitch.checked, true);
    mouseClick(enableSwitch);
    verify(!testAppSettings.mcpEnabled);
    compare(enableSwitch.checked, false);
  }

  function test_remove_rewrites_the_tokens() {
    const page = makePage();
    const controller = controllerOf(page);
    verify(controller.addToken("reader", "query") !== "");
    verify(controller.addToken("editor", "full") !== "");
    compare(tokenRows(page).length, 2);
    const noTokensLabel = findChild(page, "noTokensLabel");
    verify(noTokensLabel);
    verify(!noTokensLabel.visible);

    findChild(tokenRows(page)[0], "removeTokenButton").clicked();
    const entries = testAppSettings.mcpTokens;
    compare(entries.length, 1);
    verify(/^editor\|full\|[0-9a-f]{64}$/.test(entries[0]));
    tryCompare(noTokensLabel, "visible", false);
    compare(tokenRows(page).length, 1);
    compare(tokenRows(page)[0].title, "editor");

    // Removing the last token also turns the server off.
    verify(controller.setEnabled(true));
    verify(testAppSettings.mcpEnabled);
    findChild(tokenRows(page)[0], "removeTokenButton").clicked();
    verify(!testAppSettings.mcpEnabled);
    compare(testAppSettings.mcpTokens.length, 0);
    compare(tokenRows(page).length, 0);
    verify(noTokensLabel.visible);
  }

  function test_out_of_range_port_is_rejected() {
    const page = makePage();
    const portField = findChild(page, "mcpPortField");
    const errorLabel = findChild(page, "mcpPortError");
    verify(portField);
    verify(errorLabel);

    for (const text of ["0", "70000", "abc"]) {
      portField.text = text;
      portField.accepted();
      compare(testAppSettings.mcpPort, 7777, text);
      compare(errorLabel.visible, true, text);
      compare(portField.text, "7777", text);
    }

    // Leaving the field commits the port the same way.
    portField.forceActiveFocus();
    portField.text = "-1";
    findChild(page, "addTokenButton").forceActiveFocus();
    compare(testAppSettings.mcpPort, 7777);
    compare(errorLabel.visible, true);
    compare(portField.text, "7777");

    portField.text = "8080";
    portField.accepted();
    compare(testAppSettings.mcpPort, 8080);
    compare(errorLabel.visible, false);
    compare(portField.text, "8080");
  }

  function test_invalid_name_shows_a_message() {
    const page = makePage();
    const controller = controllerOf(page);
    verify(controller.addToken("studio", "query") !== "");
    findChild(page, "addTokenButton").clicked();
    const nameField = findChild(page, "tokenNameField");
    const errorLabel = findChild(page, "tokenNameError");
    const confirmButton = findChild(page, "addTokenConfirmButton");
    verify(nameField);
    verify(errorLabel);
    verify(confirmButton);

    for (const name of ["", "a|b", "a:b", " studio", "studio"]) {
      nameField.text = name;
      verify(errorLabel.text !== "", name);
      compare(errorLabel.visible, true, name);
      compare(confirmButton.enabled, false, name);
    }
    compare(testAppSettings.mcpTokens.length, 1);

    nameField.text = "desk";
    compare(errorLabel.visible, false);
    compare(confirmButton.enabled, true);
  }

  function test_environment_override_shows_a_notice_and_disables_the_controls() {
    const plainPage = makePage();
    const plainNotice = findChild(plainPage, "mcpOverrideNotice");
    verify(plainNotice);
    verify(!plainNotice.visible);

    testUtils.setEnvironmentVariable("ZRYTHM_MCP_TOKEN", "ci:full:hunter2");
    const activePage = makePage();
    const activeNotice = findChild(activePage, "mcpOverrideNotice");
    const activeLabel = findChild(activePage, "mcpOverrideLabel");
    verify(activeNotice);
    verify(activeLabel);
    verify(activeNotice.visible);
    verify(activeLabel.text.includes("ZRYTHM_MCP_TOKEN"));
    compare(findChild(activePage, "mcpEnabledSwitch").enabled, false);
    compare(findChild(activePage, "mcpPortField").enabled, false);
    compare(findChild(activePage, "addTokenButton").enabled, false);

    testUtils.setEnvironmentVariable("ZRYTHM_MCP_TOKEN", "ci:full:");
    const malformedPage = makePage();
    verify(findChild(malformedPage, "mcpOverrideNotice").visible);
    verify(findChild(malformedPage, "mcpOverrideLabel").text.includes("stays off"));
  }
}
