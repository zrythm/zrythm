#!/bin/bash
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# gdb exec-wrapper: runs the inferior with the Conan runtime environment
# (conanrun.sh, which also carries the sanitizer options composed by
# conanfile.py), plus gdb-specific overrides:
# - detect_leaks=0: LSan aborts under ptrace (appended last so it wins)
# - QV4_FORCE_INTERPRETER=1: the QML JIT is incompatible with ASan
#   (shadow memory vs JIT-allocated code) and crashes under TSan
#
# Usage as gdb exec-wrapper:
#   set exec-wrapper /path/to/conanrun_wrapper.sh <build_dir>
# gdb invokes: conanrun_wrapper.sh <build_dir> <program> <args...>

BUILD_DIR="$1"

if [[ ! -f "$BUILD_DIR/generators/conanrun.sh" ]]; then
  echo "Error: $BUILD_DIR/generators/conanrun.sh not found. Run 'conan install' first." >&2
  exit 1
fi
source "$BUILD_DIR/generators/conanrun.sh"

export ASAN_OPTIONS="${ASAN_OPTIONS:+${ASAN_OPTIONS}:}detect_leaks=0"
export QV4_FORCE_INTERPRETER=1

exec "${@:2}"
