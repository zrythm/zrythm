#!@BASH@
#
# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
#
# This script generates the bash/fish/zsh completion
# files.

set -ex

run_sh="@RUN_SH@"
manpage="$1"
completions_type="$2"
out_file="$3"

run_sh_dir="`dirname "$run_sh"`"

pushd "$run_sh_dir"
"$run_sh" "$manpage"
popd

cp "$run_sh_dir"/completions/$completions_type/*zrythm* \
  "$out_file"
