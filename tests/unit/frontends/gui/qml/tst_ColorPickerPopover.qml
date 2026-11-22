// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: test

  name: "ColorPickerPopover"
  when: windowShown

  Component {
    id: popoverComponent

    ColorPickerPopover {
      currentColor: "#123456"
      useColor: false
    }
  }

  SignalSpy {
    id: colorSpy
    signalName: "colorSelected"
  }
  SignalSpy {
    id: unsetSpy
    signalName: "unsetRequested"
  }

  // The picker cells in grid order: the default cell, the swatch
  // delegates, then the custom cell (the Repeater is filtered out)
  function pickerCells(popup: Popup): var {
    const result = [];
    for (const child of popup.contentItem.children) {
      if (typeof child === "object" && child !== null
          && typeof child.accessibleName === "string")
        result.push(child);
    }
    return result;
  }

  function makePopover(): Popup {
    const popup = createTemporaryObject(popoverComponent, test, {});
    colorSpy.target = popup;
    colorSpy.clear();
    unsetSpy.target = popup;
    unsetSpy.clear();
    return popup;
  }

  function test_usesSwatchColor(): void {
    const popup = makePopover();
    popup.useColor = false;
    verify(!popup.usesSwatchColor());

    popup.useColor = true;
    popup.currentColor = popup.swatches[4];
    verify(popup.usesSwatchColor());

    popup.currentColor = "#123456";
    verify(!popup.usesSwatchColor());
  }

  function test_defaultCell_selected_and_focused_when_unset(): void {
    const popup = makePopover();
    popup.useColor = false;
    popup.open();
    const cells = pickerCells(popup);

    tryVerify(() => cells[0].activeFocus);
    verify(cells[0].checked);
    for (let i = 1; i < cells.length; i++)
      verify(!cells[i].checked, "cell " + i + " must not be selected");
  }

  function test_matching_swatch_selected_and_focused(): void {
    const popup = makePopover();
    popup.useColor = true;
    popup.currentColor = popup.swatches[4];
    popup.open();
    const cells = pickerCells(popup);

    tryVerify(() => cells[5].activeFocus);
    verify(cells[5].checked);
    verify(!cells[0].checked);
  }

  function test_customCell_selected_for_non_swatch_color(): void {
    const popup = makePopover();
    popup.useColor = true;
    popup.currentColor = "#123456";
    popup.open();
    const cells = pickerCells(popup);

    tryVerify(() => cells[cells.length - 1].activeFocus);
    verify(cells[cells.length - 1].checked);
  }

  function test_clicking_swatch_emits_and_closes(): void {
    const popup = makePopover();
    popup.open();
    const cells = pickerCells(popup);
    tryVerify(() => cells[0].activeFocus);

    mouseClick(cells[3]);
    compare(colorSpy.count, 1);
    verify(Qt.colorEqual(colorSpy.signalArguments[0][0], popup.swatches[2]));
    tryVerify(() => !popup.visible);
  }

  function test_clicking_default_cell_emits_unset(): void {
    const popup = makePopover();
    popup.useColor = true;
    popup.open();
    const cells = pickerCells(popup);
    tryVerify(() => cells[cells.length - 1].activeFocus);

    mouseClick(cells[0]);
    compare(unsetSpy.count, 1);
    compare(colorSpy.count, 0);
    tryVerify(() => !popup.visible);
  }

  function test_keyboard_traversal_and_activation(): void {
    const popup = makePopover();
    popup.open();
    const cells = pickerCells(popup);
    tryVerify(() => cells[0].activeFocus);

    keyClick(Qt.Key_Right);
    tryVerify(() => cells[1].activeFocus);

    keyClick(Qt.Key_Return);
    compare(colorSpy.count, 1);
    verify(Qt.colorEqual(colorSpy.signalArguments[0][0], popup.swatches[0]));
    tryVerify(() => !popup.visible);
  }

  function test_custom_cell_keeps_popover_open(): void {
    const popup = makePopover();
    popup.useColor = true;
    popup.currentColor = "#123456";
    popup.open();
    const cells = pickerCells(popup);
    tryVerify(() => cells[cells.length - 1].activeFocus);

    // Activating the custom cell opens the color dialog without
    // emitting a selection or closing the popover
    cells[cells.length - 1].activate();
    compare(colorSpy.count, 0);
    compare(unsetSpy.count, 0);
    verify(popup.visible);

    // Close the dialog and give the window back its activation so the
    // focus tests that follow are not affected
    const dialog = findChild(popup, "colorDialog");
    verify(dialog);
    dialog.close();
    popup.close();
    test.Window.window.requestActivate();
    test.forceActiveFocus();
  }
}
