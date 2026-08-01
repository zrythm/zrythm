#!/usr/bin/env bash
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Regenerates conan.lock by resolving dependencies for all CI platform/profile
# combinations and merging them into a single lockfile.
#
# Prerequisites:
#   git submodule update --init --recursive
#   conan config install conan
#   conan remote add conan-local-index ext/conan-center-index
#   python3 tools/export_conan_recipes.py
#
# Usage:
#   ./tools/regen_conan_lockfile.sh

set -euo pipefail

LOCKFILE="conan.lock"
LOCK_OUT="$(mktemp -t conan_lock.XXXXXX.lock)"
trap 'rm -f "$LOCK_OUT"' EXIT

run() {
  local host="$1" build="$2" extra="${3:-}"
  echo ">>> Resolving -pr:h ${host} -pr:b ${build} ${extra}"
  conan lock create . \
    -pr:h "$host" -pr:b "$build" \
    --lockfile="$LOCKFILE" --lockfile-out="$LOCK_OUT" \
    --build=missing $extra
  cp "$LOCK_OUT" "$LOCKFILE"
}

echo "=== Regenerating $LOCKFILE ==="

# Remove existing lockfile so the first conan lock create resolves fresh
# (Conan auto-discovers conan.lock as default --lockfile input)
rm -f "$LOCKFILE"

# Base: Linux GCC
conan lock create . -pr:h gcc_debug   -pr:b gcc_release   --lockfile-out="$LOCKFILE" --build=missing
conan lock create . -pr:h gcc_release -pr:b gcc_release   --lockfile="$LOCKFILE" --lockfile-out="$LOCK_OUT" --build=missing
cp "$LOCK_OUT" "$LOCKFILE"

# Linux Clang
run clang_debug    gcc_release
run clang_release  gcc_release

# macOS (adds qt as build_require due to cross-building with bundled_libs)
run appleclang_debug    appleclang_build
run appleclang_release  appleclang_build

# Windows (x64)
run msvc_debug    msvc_release
run msvc_release  msvc_release

# Windows (ARM64)
run msvc_arm64_debug    msvc_arm64_release
run msvc_arm64_release  msvc_arm64_release

# Sanitizer profiles (whole-chain instrumentation)
run clang_tsan  gcc_release

echo "=== Done. $LOCKFILE updated. ==="
