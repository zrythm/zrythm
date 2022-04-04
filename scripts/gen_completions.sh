#!@BASH@
#
# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script generates the bash/fish/zsh completion
# files.

set -e

run_sh="@RUN_SH@"
manpage="$1"
completions_type="$2"
out_file="$3"

run_sh_dir="`dirname "$run_sh"`"

pushd "$run_sh_dir"
"$run_sh" $completions_type "$manpage"
popd

cp "$run_sh_dir"/completions/$completions_type/*zrythm* \
  "$out_file"
