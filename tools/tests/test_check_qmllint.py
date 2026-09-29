# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from check_qmllint import DEFAULT_MAX_ERRORS, DEFAULT_MAX_WARNINGS, filter_findings, main


def warning(path, msg="something"):
    return f"Warning: {path}:1:1: {msg} [unqualified]\n"


def error(path, msg="bad"):
    return f"Error: {path}:2:2: {msg} [missing-type]\n"


def count_by_severity(findings):
    warnings = sum(1 for line in findings if line.startswith("Warning: "))
    errors = sum(1 for line in findings if line.startswith("Error: "))
    return warnings, errors


def test_counts_warnings_and_errors_ignores_other_lines():
    lines = [
        warning("src/gui/qml/Foo.qml"),
        error("src/gui/qml/Bar.qml"),
        "Info: some hint\n",
        "  text: binding detail\n",
    ]
    assert count_by_severity(filter_findings(lines, "src/gui/qml/")) == (1, 1)


def test_path_prefix_filters_other_paths():
    lines = [
        warning("src/gui/qml/Foo.qml"),
        warning("src/other/Foo.qml"),
        error("elsewhere/Bar.qml"),
    ]
    assert count_by_severity(filter_findings(lines, "src/gui/qml/")) == (1, 0)


def test_excludes_zrythm_style_module():
    lines = [warning("src/gui/qml/ZrythmStyle/Foo.qml")]
    assert filter_findings(lines, "src/gui/qml/") == []


def test_excludes_known_false_positives():
    lines = [
        warning("src/gui/qml/Foo.qml", 'Member "x" not found on type "QObject"'),
        warning("src/gui/qml/Foo.qml", 'Member "y" not found on type "QQuickItem"'),
        warning("src/gui/qml/Foo.qml", 'Type "QColor" of property "color" not found'),
    ]
    assert filter_findings(lines, "src/gui/qml/") == []


def test_main_passes_at_or_under_thresholds(tmp_path, capsys):
    log = tmp_path / "qmllint_output.txt"
    log.write_text(warning("src/gui/qml/Foo.qml") * DEFAULT_MAX_WARNINGS)
    assert main([str(log)]) == 0
    out = capsys.readouterr().out
    assert "QML lint check passed" in out


def test_main_fails_over_warning_threshold(tmp_path, capsys):
    log = tmp_path / "qmllint_output.txt"
    log.write_text(warning("src/gui/qml/Foo.qml") * (DEFAULT_MAX_WARNINGS + 1))
    assert main([str(log)]) == 1
    assert "exceeds threshold" in capsys.readouterr().out


def test_main_fails_over_error_threshold(tmp_path, capsys):
    log = tmp_path / "qmllint_output.txt"
    log.write_text(error("src/gui/qml/Foo.qml") * (DEFAULT_MAX_ERRORS + 1))
    assert main([str(log)]) == 1
    out = capsys.readouterr().out
    assert "QML lint error count" in out
    assert "exceeds threshold" in out


def test_main_reads_stdin(monkeypatch, capsys):
    monkeypatch.setattr(
        "sys.stdin", type("Stdin", (), {"readlines": lambda self: [warning("src/gui/qml/Foo.qml")]})()
    )
    assert main(["-"]) == 0
    assert "warning count (filtered): 1" in capsys.readouterr().out


def test_main_empty_input_passes(tmp_path, capsys):
    log = tmp_path / "qmllint_output.txt"
    log.write_text("")
    assert main([str(log)]) == 0
    assert "warning count (filtered): 0" in capsys.readouterr().out
