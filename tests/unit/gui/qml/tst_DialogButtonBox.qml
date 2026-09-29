// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
  id: test

  name: "DialogButtonBox"
  height: 300
  visible: true
  when: windowShown
  width: 400

  Component {
    id: boxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }
  }

  Component {
    id: destructiveBoxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Save | DialogButtonBox.Discard | DialogButtonBox.Cancel
    }
  }

  Component {
    id: singleBoxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Ok
    }
  }

  Component {
    id: destructiveAffirmativeBoxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Discard | DialogButtonBox.Cancel
    }
  }

  Component {
    id: cancelOnlyBoxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Cancel
    }
  }

  Component {
    id: textFieldDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

      TextField {
        objectName: "dialogTextField"
        placeholderText: "Name"
      }
    }
  }

  Component {
    id: textAreaDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

      TextArea {
        objectName: "dialogTextArea"
      }
    }
  }

  Component {
    id: textEditDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

      TextEdit {
        objectName: "dialogTextEdit"
      }
    }
  }

  Component {
    id: okOnlyDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Ok
    }
  }

  Component {
    id: closeOnlyDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Close
    }
  }

  Component {
    id: mixedFooterDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Cancel

      footer: DialogButtonBox {
        defaultButton: okButton

        Button {
          id: okButton

          DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
          text: qsTr("Export")
        }
      }
    }
  }

  Component {
    id: applyFooterDialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Cancel

      footer: DialogButtonBox {
        defaultButton: applyButton

        Button {
          id: applyButton

          DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
          text: qsTr("Export")
        }
      }
    }
  }

  Component {
    id: acceptWithoutDefaultDialogComponent

    Dialog {
      footer: DialogButtonBox {
        Button {
          DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
          text: qsTr("Delete")
        }
      }
    }
  }

  Component {
    id: tallCustomBoxComponent

    DialogButtonBox {
      standardButtons: DialogButtonBox.Ok

      Button {
        implicitHeight: 40
        text: qsTr("Tall")
      }
    }
  }

  Component {
    id: dialogComponent

    Dialog {
      standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }
  }

  function buttonWithText(box, text) {
    for (let i = 0; i < box.count; ++i) {
      const button = box.itemAt(i);
      if (button.text === text)
        return button;
    }
    return null;
  }

  function test_buttons_share_the_row_equally() {
    const box = createTemporaryObject(boxComponent, test);
    verify(box);
    box.width = 300;
    verify(waitForItemPolished(box));
    const okButton = buttonWithText(box, "OK");
    const cancelButton = buttonWithText(box, "Cancel");
    verify(okButton);
    verify(cancelButton);
    tryCompare(okButton, "width", (box.availableWidth - box.spacing) / 2);
    compare(cancelButton.width, okButton.width);
    compare(okButton.width + cancelButton.width + box.spacing, box.availableWidth);
  }

  function test_single_button_spans_the_row() {
    const box = createTemporaryObject(singleBoxComponent, test);
    verify(box);
    box.width = 300;
    verify(waitForItemPolished(box));
    const okButton = box.standardButton(DialogButtonBox.Ok);
    verify(okButton);
    tryCompare(okButton, "width", box.availableWidth);
  }

  function test_accept_button_sits_trailing() {
    const box = createTemporaryObject(boxComponent, test);
    verify(box);
    box.width = 300;
    verify(waitForItemPolished(box));
    const okButton = buttonWithText(box, "OK");
    verify(okButton);
    for (let i = 0; i < box.count; ++i) {
      const button = box.itemAt(i);
      if (button !== okButton)
        verify(okButton.x > button.x, "accept button trails " + button.text);
    }
  }

  function test_accept_button_is_emphasized() {
    const box = createTemporaryObject(boxComponent, test);
    verify(box);
    box.width = 300;
    verify(waitForItemPolished(box));
    verify(buttonWithText(box, "OK").highlighted);
    verify(!buttonWithText(box, "Cancel").highlighted);
  }

  function test_accept_button_is_the_default() {
    const box = createTemporaryObject(boxComponent, test);
    verify(box);
    box.width = 300;
    tryVerify(() => box.defaultButton !== null);
    compare(box.defaultButton, box.standardButton(DialogButtonBox.Ok));
  }

  function test_destructive_button_is_never_the_default() {
    const box = createTemporaryObject(destructiveBoxComponent, test);
    verify(box);
    box.width = 400;
    tryVerify(() => box.defaultButton !== null);
    compare(box.defaultButton, box.standardButton(DialogButtonBox.Save));
  }

  function test_destructive_affirmative_leaves_no_default() {
    const box = createTemporaryObject(destructiveAffirmativeBoxComponent, test);
    verify(box);
    box.width = 300;
    tryVerify(() => box.count === 2);
    compare(box.defaultButton, null);
  }

  function test_cancel_only_box_has_no_default() {
    const box = createTemporaryObject(cancelOnlyBoxComponent, test);
    verify(box);
    box.width = 300;
    tryVerify(() => box.count === 1);
    compare(box.defaultButton, null);
  }

  SignalSpy {
    id: acceptedSpy
    signalName: "accepted"
  }

  SignalSpy {
    id: appliedSpy
    signalName: "applied"
  }

  SignalSpy {
    id: rejectedSpy
    signalName: "rejected"
  }

  FindItem {
    id: findItem
  }

  function test_return_from_a_text_field_activates_the_default() {
    const dialog = createTemporaryObject(textFieldDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    const field = findItem.byName(dialog.contentItem, "dialogTextField");
    verify(field);
    // the initial-focus heuristic puts focus in the first text field
    tryVerify(() => field.activeFocus);
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    keyClick(Qt.Key_Return);
    tryCompare(acceptedSpy, "count", 1);
  }

  function test_return_in_a_multiline_text_area_does_not_activate_the_default() {
    const dialog = createTemporaryObject(textAreaDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    const area = findItem.byName(dialog.contentItem, "dialogTextArea");
    verify(area);
    tryVerify(() => area.activeFocus);
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    keyClick(Qt.Key_Return);
    compare(acceptedSpy.count, 0);
  }

  function test_escape_without_a_cancel_dismisses_activating_nothing() {
    const dialog = createTemporaryObject(okOnlyDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    rejectedSpy.target = dialog;
    rejectedSpy.clear();
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    keyClick(Qt.Key_Escape);
    tryCompare(rejectedSpy, "count", 1);
    verify(!dialog.visible);
    compare(acceptedSpy.count, 0);
  }

  function test_disabled_default_ignores_return() {
    const box = createTemporaryObject(singleBoxComponent, test);
    verify(box);
    box.width = 300;
    tryVerify(() => box.defaultButton !== null);
    box.defaultButton.enabled = false;
    acceptedSpy.target = box;
    acceptedSpy.clear();
    box.forceActiveFocus();
    keyClick(Qt.Key_Return);
    compare(acceptedSpy.count, 0);
  }

  function test_custom_and_standard_buttons_share_the_row() {
    const dialog = createTemporaryObject(mixedFooterDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    const box = dialog.footer;
    // the dialog-level standardButtons must survive inside a custom footer
    tryVerify(() => box.count === 2);
    const cancelButton = buttonWithText(box, "Cancel");
    const exportButton = buttonWithText(box, "Export");
    verify(cancelButton);
    verify(exportButton);
    compare(box.defaultButton, exportButton);
    verify(waitForItemPolished(box));
    compare(cancelButton.width, exportButton.width);
    compare(cancelButton.width * 2 + box.spacing, box.availableWidth);
  }

  function test_close_only_dialog_has_a_close_button() {
    const dialog = createTemporaryObject(closeOnlyDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    tryVerify(() => dialog.footer.count === 1);
    verify(buttonWithText(dialog.footer, "Close"));
    // reject actions never take the default
    compare(dialog.footer.defaultButton, null);
  }

  function test_return_in_a_raw_text_edit_does_not_activate_the_default() {
    const dialog = createTemporaryObject(textEditDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    const editor = findItem.byName(dialog.contentItem, "dialogTextEdit");
    verify(editor);
    editor.forceActiveFocus();
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    keyClick(Qt.Key_Return);
    compare(acceptedSpy.count, 0);
  }

  function test_return_after_close_does_not_activate() {
    const dialog = createTemporaryObject(dialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    dialog.close();
    tryVerify(() => !dialog.visible);
    keyClick(Qt.Key_Return);
    compare(acceptedSpy.count, 0);
  }

  function test_accept_trails_in_a_triple() {
    const box = createTemporaryObject(destructiveBoxComponent, test);
    verify(box);
    box.width = 400;
    verify(waitForItemPolished(box));
    const saveButton = box.standardButton(DialogButtonBox.Save);
    const discardButton = box.standardButton(DialogButtonBox.Discard);
    const cancelButton = box.standardButton(DialogButtonBox.Cancel);
    verify(saveButton);
    verify(discardButton);
    verify(cancelButton);
    verify(discardButton.x < cancelButton.x, "destructive action leads");
    verify(cancelButton.x < saveButton.x, "cancel sits next to accept");
    // the view's current item follows the default button once the
    // platform sort has settled (the wrapper hides the view from Qt's
    // own index sync)
    const view = box.contentItem.children[0];
    compare(view.itemAtIndex(view.currentIndex), saveButton);
  }

  function test_apply_button_keeps_the_dialog_open() {
    const dialog = createTemporaryObject(applyFooterDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    tryVerify(() => dialog.footer.defaultButton !== null);
    appliedSpy.target = dialog;
    appliedSpy.clear();
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    dialog.footer.defaultButton.clicked();
    tryCompare(appliedSpy, "count", 1);
    verify(dialog.visible);
    compare(acceptedSpy.count, 0);
  }

  function test_accept_without_default_keeps_focus_off_the_row() {
    const dialog = createTemporaryObject(acceptWithoutDefaultDialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    tryVerify(() => dialog.contentItem.activeFocus);
    const box = dialog.footer;
    for (let i = 0; i < box.count; ++i)
      verify(!box.itemAt(i).activeFocus);
  }

  function test_tall_button_stretches_the_row() {
    const box = createTemporaryObject(tallCustomBoxComponent, test);
    verify(box);
    box.width = 300;
    tryVerify(() => box.count === 2);
    verify(waitForItemPolished(box));
    const okButton = box.standardButton(DialogButtonBox.Ok);
    const tallButton = buttonWithText(box, "Tall");
    verify(tallButton.height > 24);
    tryCompare(okButton, "height", tallButton.height);
  }

  function test_return_activates_the_default_button() {
    const dialog = createTemporaryObject(dialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    tryVerify(() => dialog.footer.defaultButton !== null);
    tryVerify(() => dialog.footer.defaultButton.activeFocus);
    acceptedSpy.target = dialog;
    acceptedSpy.clear();
    keyClick(Qt.Key_Return);
    tryCompare(acceptedSpy, "count", 1);
  }

  function test_escape_rejects() {
    const dialog = createTemporaryObject(dialogComponent, test);
    verify(dialog);
    dialog.open();
    tryVerify(() => dialog.visible);
    rejectedSpy.target = dialog;
    rejectedSpy.clear();
    keyClick(Qt.Key_Escape);
    tryCompare(rejectedSpy, "count", 1);
  }
}
