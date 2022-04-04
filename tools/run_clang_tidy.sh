#!/usr/bin/env sh
# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

clang_tidy_bin="$1"
meson_build_root="$2"
private_dir="$3"
src_file="$4"

compile_commands_filename="compile_commands.json"
compile_commands="$meson_build_root/$compile_commands_filename"

mkdir -p "$private_dir"
cp "$compile_commands" "$private_dir/"

sed -i -e 's/-fanalyzer//g' "$private_dir/$compile_commands_filename"

$clang_tidy_bin -p "$private_dir" "$src_file"

rm "$private_dir/$compile_commands_filename"
