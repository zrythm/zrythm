#! /bin/sh
# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This should be called from the zrythm root dir

set -e

export ZRYTHM_SKIP_PLUGIN_SCAN=1
export ZRYTHM_DSP_THREADS=1
valgrind --num-callers=160 --leak-check=full \
    --show-leak-kinds=all --track-origins=yes \
    --verbose --log-file=valog \
    --suppressions=tools/vg.sup build/src/zrythm
