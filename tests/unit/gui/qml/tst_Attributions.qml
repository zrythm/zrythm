// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest
import Zrythm

TestCase {
  id: testCase

  // The QML test runner sizes the window to the root item; without this,
  // the window is 0x0, nothing is visible and mouse events are not
  // delivered.
  height: 400
  width: 400

  name: "Attributions"

  function _findItemByObjectName(item, name) {
    if (item.objectName === name)
      return item;
    for (let i = 0; i < item.children.length; i++) {
      const found = _findItemByObjectName(item.children[i], name);
      if (found !== null)
        return found;
    }
    return null;
  }

  function test_attributions_resource_parses() {
    const jsonText = QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions.json");
    verify(jsonText.length > 0, "attributions.json must be bundled in QRC");
    const data = JSON.parse(jsonText);
    compare(data.application.name, "Zrythm");
    verify(data.components.length > 50, "expected a realistic component count");
    for (const comp of data.components) {
      verify(comp.name, "every component needs a name");
      verify(comp.license, "every component needs a license");
    }
  }

  function test_referenced_license_texts_are_bundled() {
    const data = JSON.parse(QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions.json"));
    for (const comp of data.components) {
      for (const file of comp.licenseFiles) {
        const text = QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/" + file);
        verify(text.length > 100, file + " (referenced by " + comp.name + ") must be bundled and non-trivial");
      }
    }
  }

  Component {
    id: stackViewComponent

    StackView {
    }
  }

  Component {
    id: licensesPageComponent

    AboutThirdPartyLicensesPage {
    }
  }

  function test_page_lists_and_filters_components() {
    const stackView = createTemporaryObject(stackViewComponent, testCase);
    verify(stackView);
    const page = createTemporaryObject(licensesPageComponent, testCase, {
      stackView: stackView
    });
    verify(page);
    const searchField = _findItemByObjectName(page, "searchField");
    verify(searchField);
    const listView = _findItemByObjectName(page, "licensesListView");
    verify(listView);
    const data = JSON.parse(QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions.json"));
    tryCompare(listView, "count", data.components.length);
    searchField.text = "rubber";
    tryVerify(() => listView.count >= 1);
    tryVerify(() => listView.itemAtIndex(0) !== null);
    for (let i = 0; i < listView.count; i++) {
      const shown = JSON.parse(listView.itemAtIndex(i).json);
      verify(shown.name.toLowerCase().includes("rubber"),
        "filtered view must only show matches, got " + shown.name);
    }
    searchField.text = "";
    tryCompare(listView, "count", data.components.length);
  }

  function test_detail_page_shows_full_license_text() {
    const stackView = createTemporaryObject(stackViewComponent, testCase);
    const page = createTemporaryObject(licensesPageComponent, testCase, {
      stackView: stackView
    });
    verify(page);
    page.width = 400;
    page.height = 400;
    stackView.push(page);
    const listView = _findItemByObjectName(page, "licensesListView");
    verify(listView);
    tryVerify(() => listView.count > 0);
    tryVerify(() => listView.itemAtIndex(0) !== null);
    const delegate = listView.itemAtIndex(0);
    const comp = JSON.parse(delegate.json);
    verify(Array.isArray(comp.licenseFiles), "detail data must carry a real array");
    page.openComponent(comp);
    tryCompare(stackView, "depth", 2);
    const textArea = _findItemByObjectName(stackView.currentItem, "licenseTextArea");
    verify(textArea);
    tryVerify(() => textArea.text.length > 100);
    verify(!textArea.text.includes("License text not bundled"),
      "detail page must show a real license text, got the fallback message");
  }

  function _showTestCaseItem() {
    // The QML test runner keeps the root TestCase item hidden; show it so
    // hit-testing delivers mouse events to items under it.
    testCase.visible = true;
    tryVerify(() => testCase.visible);
  }

  function test_licenses_page_scrollbar_drag_moves_content_live() {
    const stackView = createTemporaryObject(stackViewComponent, testCase);
    const page = createTemporaryObject(licensesPageComponent, testCase, {
      stackView: stackView
    });
    verify(page);
    stackView.width = 400;
    stackView.height = 400;
    page.width = 400;
    page.height = 400;
    _showTestCaseItem();
    stackView.push(page);
    tryVerify(() => page.visible);
    const listView = _findItemByObjectName(page, "licensesListView");
    verify(listView);
    const data = JSON.parse(QmlUtils.readTextFileContent(":/qt/qml/Zrythm/licenses/attributions.json"));
    tryCompare(listView, "count", data.components.length);
    tryVerify(() => listView.contentHeight > listView.height);
    const bar = listView.ScrollBar.vertical;
    verify(bar && bar.size > 0 && bar.size < 1);
    const before = listView.contentY;
    const handleX = bar.width / 2;
    const handleY = (bar.position + bar.size / 2) * bar.height;
    mousePress(bar, handleX, handleY);
    verify(bar.pressed, "press must grab the scrollbar handle");
    mouseMove(bar, handleX, handleY + 30, -1, Qt.LeftButton);
    verify(listView.contentY > before + 5,
      "content must follow the scrollbar drag before release (was "
      + before + ", mid-drag " + listView.contentY + ")");
    mouseRelease(bar, handleX, handleY + 30);
  }
}
