// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import Zrythm

TestCase {
  id: test

  name: "PreferencesPathListEditorDialog"
  height: 400
  visible: true
  when: windowShown
  width: 560

  Component {
    id: dialogComponent

    PreferencesPathListEditorDialog {
    }
  }

  SignalSpy {
    id: acceptedPathsSpy

    signalName: "pathsAccepted"
  }

  function makeDialog(): PreferencesPathListEditorDialog {
    const dialog = createTemporaryObject(dialogComponent, test);
    verify(dialog);
    acceptedPathsSpy.target = dialog;
    acceptedPathsSpy.clear();
    return dialog;
  }

  function test_open_copies_paths() {
    const dialog = makeDialog();
    const sourcePaths = ["/tmp/a", "/tmp/b"];
    dialog.openWithPaths(sourcePaths);
    compare(dialog.paths, sourcePaths);

    verify(dialog.addPath("/tmp/c"));
    compare(sourcePaths.length, 2);
    compare(dialog.paths, ["/tmp/a", "/tmp/b", "/tmp/c"]);
  }

  function test_duplicate_paths_are_rejected() {
    const dialog = makeDialog();
    dialog.openWithPaths(["/tmp/a"]);
    verify(!dialog.addPath("/tmp/a"));
    compare(dialog.paths, ["/tmp/a"]);
  }

  function test_remove_path() {
    const dialog = makeDialog();
    dialog.openWithPaths(["/tmp/a", "/tmp/b", "/tmp/c"]);
    dialog.removePath(1);
    compare(dialog.paths, ["/tmp/a", "/tmp/c"]);
  }

  function test_accept_emits_edited_paths() {
    const dialog = makeDialog();
    dialog.openWithPaths(["/tmp/a"]);
    dialog.addPath("/tmp/b");
    dialog.removePath(0);
    dialog.accept();
    compare(acceptedPathsSpy.count, 1);
    compare(acceptedPathsSpy.signalArguments[0][0], ["/tmp/b"]);
  }
}
