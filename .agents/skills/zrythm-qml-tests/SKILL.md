<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

---
name: zrythm-qml-tests
description: >-
  Run and filter Zrythm's Qt Quick Test suite (zrythm_gui_qml_unit_tests):
  binary path, offscreen flag, the TestCaseName::function filter syntax,
  -functions listing, ctest integration, and color-assertion gotchas.
  Use when running, debugging, or adding QML tests in this repository.
---

# Zrythm QML tests

## Layout & discovery

- Test files are `tst_*.qml`, flat under `tests/unit/gui/qml/` — one
  `TestCase` per file, named via its `name:` property (e.g. a file
  `tst_TrackColors.qml` declares `name: "TrackColors"`).
- The runner is built from `qml_test_main.cpp` with `QUICK_TEST_MAIN`:
  it discovers `tst_*.qml` from the source directory **at runtime**.
  Consequences:
  - adding a new `tst_Foo.qml` needs **no CMake registration**
  - editing an existing test file needs **no rebuild**
  - only C++-side changes require rebuilding the target
    (`cmake --build conanbuild/Debug --target zrythm_gui_qml_unit_tests`)

## Running

```bash
# Directly (fast iteration on one case)
./conanbuild/Debug/products/bin/zrythm_gui_qml_unit_tests -platform offscreen [filters...]

# List every available test function (verify discovery + exact names)
./conanbuild/Debug/products/bin/zrythm_gui_qml_unit_tests -platform offscreen -functions

# Via CTest (the CI path, runs everything)
ctest --test-dir conanbuild/Debug -R zrythm_gui_qml_unit_tests --output-on-failure
```

## Filter syntax (the trap)

Filters are `TestCaseName::functionName`, where `TestCaseName` is the
TestCase's `name:` property — NOT the file name and NOT the bare
function name:

```bash
# right
./conanbuild/Debug/products/bin/zrythm_gui_qml_unit_tests -platform offscreen TrackColors::test_dark_defaults
# wrong (silently matches nothing: "Could not find the following test functions")
./conanbuild/Debug/products/bin/zrythm_gui_qml_unit_tests -platform offscreen tst_TrackColors
./conanbuild/Debug/products/bin/zrythm_gui_qml_unit_tests -platform offscreen test_dark_defaults
```

Multiple filters can be passed as separate arguments. A failing
`verify()` aborts the rest of the test function — to observe every
row of a table-driven test, dump the actuals with `console.info()`
in a temporary test function instead of iterating assertions.

## Writing color assertions

- Compare colors with a ±1/255 tolerance helper (see `compareNear` in
  `tst_ZrythmTheme.qml`): QColor quantizes through 16-bit channels, so
  exact equality fails spuriously.
- Colors printed by QML (`console.info(color)` etc.) are `#aarrggbb`
  with an `ff` alpha prefix — strip the first two digits to get the
  familiar `#rrggbb`.
- Contrast and color-space assertions can call the `ColorMath`
  singleton (`contrastRatio(a, b)`, `oklchColor(l, c, h)`,
  `rgbToOklch(color)`).

## Imports in test files

App singletons and registered C++ types resolve with `import Zrythm`
(the `QmlTests` module imports `zrythm_qml_lib`); the theme with
`import ZrythmStyle`; `QtTest` for `TestCase`.

## Benign noise

- `Fontconfig error: Cannot load default config file` on headless runs
- `QML debugging is enabled. Only use this in a safe environment.`
